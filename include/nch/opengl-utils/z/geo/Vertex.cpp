#include "Vertex.h"
#include <sstream>
#include <iomanip>

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
Vertex::Vertex(const nch::Vec3f& pos)
: Vertex(pos, glm::vec3(0.0f)){}

Vertex::Vertex(float x, float y, float z)
: Vertex(glm::vec3(x, y, z)){}

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