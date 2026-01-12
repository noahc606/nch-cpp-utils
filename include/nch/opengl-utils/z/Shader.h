#pragma once
#include <GL/glew.h>
#include <string>

namespace nch { class Shader {
public:
    Shader(std::string vertexCode, std::string fragmentCode);
    ~Shader();
    static Shader* readFromFiles(std::string vertexFilePath, std::string fragmentFilePath);
    static Shader* readFromAssetPath(std::string assetPath);
    static Shader* createDefault2D_TexOrSolid();
    static Shader* createDefault2D_Tex();
    static Shader* createDefault3D();

    GLuint getID();

    void useProgram();
    void deleteProgram();
private:
    void compileErrors(GLuint shader, std::string type);

    GLuint id;
}; }