#pragma once
#include <glm/glm.hpp>
#include <nch/math-utils/vec3.h>

class Vertex {
public:
    Vertex();
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec3& col, const glm::vec2& uv);
    Vertex(const glm::vec3& pos, const glm::vec3& col, const glm::vec2& uv);
    Vertex(const glm::vec3& pos, const glm::vec3& col);
    Vertex(const glm::vec3& pos, const glm::vec2& uv);
    Vertex(const glm::vec3& pos);
    Vertex(const nch::Vec3f& pos);
    Vertex(float x, float y, float z);

    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texUV;
private:
};