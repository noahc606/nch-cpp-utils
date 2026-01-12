#include "Vertex.h"

Vertex::Vertex(){}
Vertex::Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec3& col, const glm::vec2& uv) {
    Vertex::pos = pos;
    Vertex::normal = norm;
    Vertex::color = col;
    Vertex::texUV = uv;
}
Vertex::Vertex(const glm::vec3& pos, const glm::vec3& col, const glm::vec2& uv) {
    Vertex::pos = pos;
    Vertex::normal = glm::vec3(0.0f);
    Vertex::color = col;
    Vertex::texUV = uv;
}
Vertex::Vertex(const glm::vec3& pos, const glm::vec2& uv) {
    Vertex::pos = pos;
    Vertex::normal = glm::vec3(0.0f);
    Vertex::color = glm::vec3(1.0f);
    Vertex::texUV = uv;
}
Vertex::Vertex(const glm::vec3& pos, const glm::vec3& col)
: Vertex(pos, glm::vec3(0.0f), col, glm::vec2(0.0f)){}

Vertex::Vertex(const glm::vec3& pos)
: Vertex(pos, glm::vec3(0.0f)){}

Vertex::Vertex(float x, float y, float z)
: Vertex(glm::vec3(x, y, z)){}