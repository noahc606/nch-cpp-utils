#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <nch/cpp-utils/file-utils.h>
#include <nch/cpp-utils/log.h>

using namespace nch;

Shader::Shader(std::string vtxCode, std::string frgCode)
{
    const char* vtxSrc = vtxCode.c_str();
    const char* frgSrc = frgCode.c_str();

    /* Build shader program */ {
        //Vertex shader
        GLuint glVtxShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(glVtxShader, 1, &vtxSrc, NULL);
        glCompileShader(glVtxShader);
        compileErrors(glVtxShader, "VERTEX");

        //Fragment shader
        GLuint glFrgShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(glFrgShader, 1, &frgSrc, NULL);
        glCompileShader(glFrgShader);
        compileErrors(glFrgShader, "FRAGMENT");

        //Create shader program
        id = glCreateProgram();
        glAttachShader(id, glVtxShader);
        glAttachShader(id, glFrgShader);
        glLinkProgram(id);

        //Cleanup (we already have the program by now)
        glDeleteShader(glVtxShader);
        glDeleteShader(glFrgShader);
    }
}

Shader* Shader::readFromFiles(std::string vertexFilePath, std::string fragmentFilePath) {
    std::string vtxCode = FileUtils::readFileContent(vertexFilePath);
    std::string frgCode = FileUtils::readFileContent(fragmentFilePath);
    return new Shader(vtxCode, frgCode);
}
Shader* Shader::readFromAssetPath(std::string assetPath) {
    return readFromFiles(assetPath+".vs", assetPath+".fs");
}
Shader* Shader::createDefault2D_TexOrSolid() {
    return new Shader(
        R"(
            #version 330 core
            layout(location = 0) in vec2 inPos;     //Original position
            layout(location = 1) in vec4 inColor;   //Original color
            layout(location = 2) in vec2 inTexUV;   //Original texture UV
            out vec4 vColor;                        //Final vertex color -> Frag
            out vec2 vTexUV;                        //Final texture UV -> Frag
            uniform mat4 uProjection;               //Projection matrix

            void main() {
                vColor = inColor;
                vTexUV = inTexUV;
                gl_Position = uProjection*vec4(inPos, 0.0, 1.0);
            }
        )",
        R"(
            #version 330 core
            in vec4 vColor;             //Vertex -> Color
            in vec2 vTexUV;             //Vertex -> UV
            out vec4 FragColor;         //Final pixel color
            uniform bool uApplyTexture; //Apply texture?
            uniform sampler2D uTexture;

            void main() {
                if(uApplyTexture) {
                    FragColor = texture(uTexture, vTexUV)*vColor;
                } else {
                    FragColor = vColor;
                }
            }
        )"
    );
}
Shader* Shader::createDefault3D() {
    return new Shader(
        R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;    //Position of vertices
            layout (location = 1) in vec3 aNormal; //Normal vector of triangle/face (used in light calculation)
            layout (location = 2) in vec3 aColor;  //Color of vertices
            layout (location = 3) in vec2 aTex;    //Texture coordinate of vertices

            uniform mat4 camMatrix;
            uniform mat4 model;

            // -> to .fs...
            out vec3 currPos;
            out vec3 Normal;
            out vec3 color;
            out vec2 texCoord;

            void main()
            {
                currPos = vec3(model * vec4(aPos, 1.0f)); //Assign the current position from our vertex data to "currPos" ->.fs
                Normal = aNormal;                         //... to "Normal" ->.fs
                color = aColor;                           //... to "color" -> .fs
                texCoord = aTex;                          //... to "texCoord" -> .fs

                //Output the finalized positions of all vertices
                gl_Position = camMatrix * vec4(currPos, 1.0);
            }
        )",
        R"(
            #version 330 core

            // -> from .vs
            in vec3 currPos;
            in vec3 Normal;
            in vec3 color;
            in vec2 texCoord;
            out vec4 FragColor;

            void main()
            {
                vec4 modColor;
                modColor.r = color.r;
                modColor.g = color.g;
                modColor.b = color.b;
                modColor.a = 255;

                FragColor = modColor;
            }
        )"
    );
}

Shader::~Shader() {
    deleteProgram();
}

GLuint Shader::getID() {
    return id;
}
GLint Shader::getUniformLoc(const std::string& name) {
    return glGetUniformLocation(id, name.c_str());
}
void Shader::setModelMatrix(Vec3f scale, Vec3f rotation, Vec3f centerOfRotation) {
    glm::mat4 objectModel = glm::mat4(1.f);
    //Translate to center of rotation
    objectModel = glm::translate(objectModel, glm::vec3(centerOfRotation.x, centerOfRotation.y, centerOfRotation.z));
    //Apply rotations (in order: Z, Y, X) - rotation is in degrees
    objectModel = glm::rotate(objectModel, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    objectModel = glm::rotate(objectModel, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    objectModel = glm::rotate(objectModel, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    //Apply scale
    objectModel = glm::scale(objectModel, glm::vec3(scale.x, scale.y, scale.z));
    //Translate back from center of rotation
    objectModel = glm::translate(objectModel, glm::vec3(-centerOfRotation.x, -centerOfRotation.y, -centerOfRotation.z));
    glUniformMatrix4fv(getUniformLoc("model"), 1, GL_FALSE, glm::value_ptr(objectModel));
    //Calculate and set normal matrix (inverse transpose of upper-left 3x3)
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(objectModel)));
    glUniformMatrix3fv(getUniformLoc("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
}
void Shader::setModelMatrix(Vec3f scale) {
    setModelMatrix(scale, {0}, {0});
}
void Shader::resetModelMatrix() {
    setModelMatrix({1});
}

void Shader::useProgram()
{
    glUseProgram(id);
}
void Shader::deleteProgram()
{
    glDeleteProgram(id);
}

void Shader::compileErrors(GLuint shader, std::string type)
{
    GLint hasCompiled;
    char infoLog[1024];
    if(type!="PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if(hasCompiled==GL_FALSE) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            Log::errorv(__PRETTY_FUNCTION__, "Shader compilation error", "%s", type.c_str());
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if(hasCompiled==GL_FALSE) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            Log::errorv(__PRETTY_FUNCTION__, "Shader linking error", "%s", type.c_str());
        }
    }
}