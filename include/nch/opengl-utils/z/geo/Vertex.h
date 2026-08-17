#pragma once
#include <nch/math-utils/vec2.h>
#include <nch/math-utils/vec3.h>
#include <string>

namespace nch { class Vertex {
public:
    Vertex();
    Vertex(const Vec3f& pos, const Vec3f& norm, const Vec3f& col, const Vec2f& uv);
    Vertex(const Vec3f& pos, const Vec3f& col, const Vec2f& uv);
    Vertex(const Vec3f& pos, const Vec3f& col);
    Vertex(const Vec3f& pos, const Vec2f& uv);
    Vertex(const Vec3f& pos);
    Vertex(float x, float y, float z);

    std::string toString() const;

    //Layout is read by Mesh's VBO setup via offsetof/sizeof, so these must stay standard-layout floats.
    Vec3f pos;
    Vec3f normal;
    Vec3f color;
    Vec2f texUV;
    Vec3f light;
private:
}; }
