#include "Vertex.h"
#include <sstream>
#include <iomanip>
using namespace nch;

Vertex::Vertex(){
    light = Vec3f(0.0f);
}
Vertex::Vertex(const Vec3f& pos, const Vec3f& norm, const Vec3f& col, const Vec2f& uv) {
    Vertex::pos = pos;
    Vertex::normal = norm;
    Vertex::color = col;
    Vertex::texUV = uv;
    Vertex::light = Vec3f(0.0f);
}
Vertex::Vertex(const Vec3f& pos, const Vec3f& col, const Vec2f& uv) {
    Vertex::pos = pos;
    Vertex::normal = Vec3f(0.0f);
    Vertex::color = col;
    Vertex::texUV = uv;
    Vertex::light = Vec3f(0.0f);
}
Vertex::Vertex(const Vec3f& pos, const Vec2f& uv) {
    Vertex::pos = pos;
    Vertex::normal = Vec3f(0.0f);
    Vertex::color = Vec3f(1.0f);
    Vertex::texUV = uv;
    Vertex::light = Vec3f(0.0f);
}
Vertex::Vertex(const Vec3f& pos, const Vec3f& col)
: Vertex(pos, Vec3f(0.0f), col, Vec2f(0.0f)){}

Vertex::Vertex(const Vec3f& pos)
: Vertex(pos, Vec3f(0.0f)){}

Vertex::Vertex(float x, float y, float z)
: Vertex(Vec3f(x, y, z)){}

std::string Vertex::toString() const {
    auto compact = [](float f) {
        std::string s = std::to_string(f);
        s.erase(s.find_last_not_of('0')+1, std::string::npos);
        if(s.back()=='.') s.pop_back();
        return s;
    };

    std::stringstream ss;
    ss << "V[p:(" << compact(pos.x) << "," << compact(pos.y) << "," << compact(pos.z) << ")";
    ss << " n:(" << compact(normal.x) << "," << compact(normal.y) << "," << compact(normal.z) << ")";
    if(color.x!=1.0f || color.y!=1.0f || color.z!=1.0f) {
        ss << " c:(" << compact(color.x) << "," << compact(color.y) << "," << compact(color.z) << ")";
    }
    ss << " uv:(" << compact(texUV.x) << "," << compact(texUV.y) << ")]";
    return ss.str();
}
