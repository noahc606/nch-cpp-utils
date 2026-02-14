#include "Poly.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <sstream>
#include <stdexcept>
#include "Tri.h"
#include "Quad.h"
#include "GeoUtils.h"
#include "Camera3D.h"
using namespace nch;

Poly::Poly(){}
Poly::Poly(const std::vector<Vertex>& verts)
{
    Poly::verts = verts;
    super_updateNormals();
}
Poly::~Poly(){}

int Poly::getType() const {
    return type;
}
int Poly::getNumVerts() const {
    return verts.size();
}
int Poly::vs() const {
    return getNumVerts();
}
Vertex Poly::v(int idx) const {
    return verts[idx];
}
glm::vec3 Poly::norm() const {
    return normal;
}
std::string Poly::toString() const {
    std::stringstream ss;
    ss << "Poly{";
    for(size_t i = 0; i<verts.size(); i++) {
        if(i>0) ss << ",";
        ss << verts[i].toString();
    }
    ss << "}";
    return ss.str();
}

void Poly::simplyTex(const glm::vec2& uv0, const glm::vec2& uv1)
{
    if(verts.size()<3) return;

    //Determine dominant axis (6 planes: +X, -X, +Y, -Y, +Z, -Z)
    glm::vec3 absNorm = glm::abs(normal);
    int dominantAxis = 0; //0=X, 1=Y, 2=Z
    if(absNorm.y>=absNorm.x && absNorm.y>=absNorm.z) {
        dominantAxis = 1;
    } else if(absNorm.z>=absNorm.x && absNorm.z>=absNorm.y) {
        dominantAxis = 2;
    }

    //Project vertices to 2D based on plane
    std::vector<int> polyDirs;
    std::vector<glm::vec2> points2D;
    points2D.reserve(verts.size());
    for(size_t i = 0; i<verts.size(); i++) {
        switch(dominantAxis) {
            case 0: {
            if(normal.x>0) { points2D.push_back(glm::vec2(verts[i].pos.y, verts[i].pos.z)); polyDirs.push_back(Camera3D::WEST); }
            else           { points2D.push_back(glm::vec2(verts[i].pos.y, verts[i].pos.z)); polyDirs.push_back(Camera3D::EAST); }
            } break;
            case 1: {
            if(normal.y>0) { points2D.push_back(glm::vec2(verts[i].pos.x, verts[i].pos.z)); polyDirs.push_back(Camera3D::UP); }
            else           { points2D.push_back(glm::vec2(verts[i].pos.x, verts[i].pos.z)); polyDirs.push_back(Camera3D::DOWN); }
            } break;
            case 2: {
            if(normal.z>0) { points2D.push_back(glm::vec2(verts[i].pos.x, verts[i].pos.y)); polyDirs.push_back(Camera3D::NORTH); }
            else           { points2D.push_back(glm::vec2(verts[i].pos.x, verts[i].pos.y)); polyDirs.push_back(Camera3D::SOUTH); }
            } break;
        }
    }
    if(points2D.size()<3) {
        throw std::logic_error("points2D incorrectly populated");
    }

    //Find UV grid
    float minU = points2D[0].x, minV = points2D[0].y;
    float maxU = points2D[0].y, maxV = points2D[0].y;
    for(size_t i = 0; i<verts.size(); i++) {
        const float& u = points2D[i].x;
        const float& v = points2D[i].y;
        if(u<minU) minU = u; if(v<minV) minV = v;
        if(u>maxU) maxU = u; if(v>maxV) maxV = v;
    }
    float uGrid = std::floor(minU), vGrid = std::floor(minV);

    //Map each vertex to UV coordinates
    for(size_t i = 0; i<verts.size(); i++) {
        //Normalize to [0,1] range (wrap via modulo)
        float u = points2D[i].x-uGrid;
        float v = points2D[i].y-vGrid;

        //Remap to [uv0, uv1] range
        verts[i].texUV.x = uv0.x + u * (uv1.x - uv0.x);
        verts[i].texUV.y = uv0.y + v * (uv1.y - uv0.y);

        //Set vertex color to white
        verts[i].color = glm::vec3(1.0f, 1.0f, 1.0f);
    }
}
void Poly::rotate(const glm::vec3& center, const glm::vec3& xyzRot)
{
    for(size_t i = 0; i<verts.size(); i++) {
        GeoUtils::rotatePoint(verts[i].pos, center, xyzRot);
    }
    super_updateNormals();
}
void Poly::move(const glm::vec3& offset)
{
    for(size_t i = 0; i<verts.size(); i++) {
        verts[i].pos += offset;
    }
}
void Poly::invertNorm() {
    normal = -normal;
    for(size_t i = 0; i<verts.size(); i++) {
        verts[i].normal = normal;
    }
}

//Helper: Project 3D point to 2D based on dominant axis
static glm::vec2 projectTo2D(const glm::vec3& p, const glm::vec3& norm) {
    //Choose projection plane based on dominant normal component
    glm::vec3 absNorm = glm::abs(norm);
    if(absNorm.x>=absNorm.y && absNorm.x>=absNorm.z) {
        //Project onto YZ plane
        return glm::vec2(p.y, p.z);
    } else if(absNorm.y>=absNorm.x && absNorm.y>=absNorm.z) {
        //Project onto XZ plane
        return glm::vec2(p.x, p.z);
    } else {
        //Project onto XY plane
        return glm::vec2(p.x, p.y);
    }
}

//Helper: Calculate signed area of triangle (2D)
static float triangleArea2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
    return (b.x-a.x)*(c.y-a.y) - (c.x-a.x)*(b.y-a.y);
}

//Helper: Check if point p is inside triangle abc (2D)
static bool pointInTriangle2D(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
    float area = triangleArea2D(a, b, c);
    float s1 = triangleArea2D(p, a, b);
    float s2 = triangleArea2D(p, b, c);
    float s3 = triangleArea2D(p, c, a);

    //All signs must match the triangle's winding
    if(area>=0) {
        return s1>=0 && s2>=0 && s3>=0;
    } else {
        return s1<=0 && s2<=0 && s3<=0;
    }
}

//Helper: Check if three consecutive vertices form a valid ear
static bool isEar(const std::vector<int>& indices, const std::vector<glm::vec2>& points2D, int prev, int curr, int next) {
    const glm::vec2& a = points2D[indices[prev]];
    const glm::vec2& b = points2D[indices[curr]];
    const glm::vec2& c = points2D[indices[next]];

    //Check if triangle is convex (positive winding)
    float area = triangleArea2D(a, b, c);
    if(area<=0) return false; //Reflex vertex

    //Check if any other vertex is inside this triangle
    for(size_t i = 0; i<indices.size(); i++) {
        if(i==prev || i==curr || i==next) continue;
        if(pointInTriangle2D(points2D[indices[i]], a, b, c)) {
            return false;
        }
    }

    return true;
}

std::vector<Poly> Poly::split() const {
    int n = verts.size();

    //Throw exception for degenerate polygons (<=2 vertices)
    if(n<=2) {
        throw std::invalid_argument("Cannot split polygon with 2 or fewer vertices");
    }

    //Return single-element array for triangles
    if(n==3) {
        std::vector<Poly> result;
        result.push_back(*this);
        return result;
    }

    //Project vertices to 2D for ear clipping
    std::vector<glm::vec2> points2D;
    points2D.reserve(n);
    for(int i = 0; i<n; i++) {
        points2D.push_back(projectTo2D(verts[i].pos, normal));
    }

    //Ear clipping algorithm
    std::vector<Poly> result;
    result.reserve(n-2);

    //Create index list for remaining vertices
    std::vector<int> indices;
    indices.reserve(n);
    for(int i = 0; i<n; i++) {
        indices.push_back(i);
    }

    //Clip ears until only 3 vertices remain
    while(indices.size()>3) {
        bool earFound = false;

        for(size_t i = 0; i<indices.size(); i++) {
            int prev = (i==0) ? indices.size()-1 : i-1;
            int curr = i;
            int next = (i==indices.size()-1) ? 0 : i+1;

            if(isEar(indices, points2D, prev, curr, next)) {
                //Create triangle from this ear
                Tri tri(verts[indices[prev]], verts[indices[curr]], verts[indices[next]]);
                result.push_back(tri);

                //Remove the ear tip vertex
                indices.erase(indices.begin() + curr);
                earFound = true;
                break;
            }
        }

        if(!earFound) {
            //Fallback: this shouldn't happen for valid simple polygons
            //but use fan triangulation as a safety measure
            result.clear();
            result.reserve(n-2);
            for(int i = 1; i<n-1; i++) {
                Tri tri(verts[0], verts[i], verts[i+1]);
                result.push_back(tri);
            }
            return result;
        }
    }

    //Add final triangle
    if(indices.size()==3) {
        Tri tri(verts[indices[0]], verts[indices[1]], verts[indices[2]]);
        result.push_back(tri);
    }

    return result;
}

void Poly::super_type(int type) {
    if(Poly::type==PolyType::UNKNOWN) {
        Poly::type = type;
    } else {
        assert(false);
    }
}
void Poly::super_updateNormals() {

    if(verts.size()==3) {
        normal = glm::cross((v(1).pos-v(0).pos), (v(2).pos-v(0).pos));
        verts[0].normal = normal;
        verts[1].normal = normal;
        verts[2].normal = normal;
    }
    if(verts.size()==4) {
        normal = Tri(v(0), v(1), v(2)).norm();
        verts[0].normal = normal;
        verts[1].normal = normal;
        verts[2].normal = normal;
        verts[3].normal = normal;
    }
}
void Poly::super_updateColors() {
    glm::vec3 polyCol = {1.0, 0.0, 0.0};

}