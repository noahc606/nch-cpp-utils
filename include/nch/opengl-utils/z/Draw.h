#pragma once
#include <GL/glew.h>
#include <SDL2/SDL_ttf.h>
#include <glm/fwd.hpp>
#include <nch/cpp-utils/color.h>
#include <nch/math-utils/vec2.h>
#include <nch/math-utils/vec3.h>
#include <string>
#include "Shader.h"

namespace nch { class Draw {
public:
    static void setShader2D(Shader* shader2D);

    //Textured geometry
    static void texturedTri(const nch::Vec3f& p0, const nch::Vec3f& p1, const nch::Vec3f& p2, const nch::Vec2f& t1, const nch::Vec2f& t2, const nch::Vec2f& t3, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);
    static void texturedTri(const nch::Vec3f& p0, const nch::Vec3f& p1, const nch::Vec3f& p2, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);
    static void texturedTri(const nch::Vec2f& p0, const nch::Vec2f& p1, const nch::Vec2f& p2, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);
    static void texturedQuad(const nch::Vec3f& p0, const nch::Vec3f& p1, const nch::Vec3f& p2, const nch::Vec3f& p3, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);

    //Solid-color geometry
    static void tri(const nch::Vec3f& p0, const nch::Vec3f& p1, const nch::Vec3f& p2, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);
    static void tri(const nch::Vec2f& p0, const nch::Vec2f& p1, const nch::Vec2f& p2, GLuint& glVAO, GLuint& glVBO, GLuint& glEBO);

    //Miscellaneous
    static void setTexModColor(const nch::Color& mod);
    static void streamText(const std::string& drawnText, TTF_Font* font, const nch::Color& color, float x, float y, GLuint projectionUniform, glm::mat4 projectionMatrix);
    static void streamLine3D(const nch::Color& col, glm::dvec3 p0, glm::dvec3 p1);
    static void streamQuad3D(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
    static void streamQuad3D(const nch::Color& col, glm::dvec3 p0, glm::dvec3 p1, glm::dvec3 p2, glm::dvec3 p3);
    
private:
    static nch::Color mod;
    static Shader* shader2D;
}; }