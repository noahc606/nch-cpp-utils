#pragma once
#include <GL/glew.h>
#include "nch/opengl-utils/geo/vertex.h"
#include <vector>

class EBO {
public:
    static GLuint build(std::vector<GLuint>& idxes);
    static void destroy(GLuint glEBO);

    static void bind(GLuint glEBO);
    static void unbind();
    static void update(GLuint glEBO, GLuint idx, const GLuint& vtxIdx);
private:
};

class VAO {
public:
    static GLuint build();
    static void destroy(GLuint glVAO);

    static void linkAttrib(GLuint glVBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);
    static void bind(GLuint glVAO);
    static void unbind();
private:
};

class VBO {
public:
    static GLuint build(std::vector<Vertex>& vertices);
    static void destroy(GLuint glVBO);

    static void bind(GLuint glVBO);
    static void unbind();
    static void update(GLuint glVBO, GLuint idx, const Vertex& newVtx);
};