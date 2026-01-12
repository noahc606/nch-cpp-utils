#include "BufferObj.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLuint EBO::build(std::vector<GLuint>& idxes)
{
    GLuint res;
    glGenBuffers(1, &res);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, res);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxes.capacity()*sizeof(GLuint), idxes.data(), GL_STATIC_DRAW);
    return res;
}
void EBO::destroy(GLuint glEBO) {
    glDeleteBuffers(1, &glEBO);
}

void EBO::bind(GLuint glEBO) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glEBO);
}
void EBO::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void EBO::update(GLuint glEBO, GLuint idx, const GLuint& vtxIdx) {
    EBO::bind(glEBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, idx*sizeof(GLuint), sizeof(GLuint), &vtxIdx);
    EBO::unbind();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


GLuint VAO::build() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    return vao;
}

void VAO::linkAttrib(GLuint glVBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
{
    VBO::bind(glVBO);
    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    VBO::unbind();
}

void VAO::bind(GLuint glVAO) {
    glBindVertexArray(glVAO);
}

void VAO::unbind() {
    glBindVertexArray(0);
}

void VAO::destroy(GLuint glVAO) {
    glDeleteVertexArrays(1, &glVAO);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


GLuint VBO::build(std::vector<Vertex>& vertices)
{
    GLuint res;
    glGenBuffers(1, &res);
    glBindBuffer(GL_ARRAY_BUFFER, res);
    glBufferData(GL_ARRAY_BUFFER, vertices.capacity()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    return res;
}
void VBO::destroy(GLuint glVBO) {
    glDeleteBuffers(1, &glVBO);
}

void VBO::bind(GLuint glVBO) {
    glBindBuffer(GL_ARRAY_BUFFER, glVBO);
}
void VBO::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void VBO::update(GLuint glVBO, GLuint idx, const Vertex& newVtx) {
    VBO::bind(glVBO);
    glBufferSubData(GL_ARRAY_BUFFER, idx*sizeof(Vertex), sizeof(Vertex), &newVtx);
    VBO::unbind();
}