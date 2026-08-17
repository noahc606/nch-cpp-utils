#include "GeoUtils.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace nch;

const std::vector<Poly> GeoUtils::unitCube = {
    Poly({{0,0,0},{0,0,1},{0,1,1},{0,1,0}}), //East
    Poly({{1,0,0},{1,1,0},{1,1,1},{1,0,1}}), //West
    Poly({{0,0,1},{0,0,0},{1,0,0},{1,0,1}}), //Down
    Poly({{1,1,1},{1,1,0},{0,1,0},{0,1,1}}), //Up
    Poly({{1,1,0},{1,0,0},{0,0,0},{0,1,0}}), //South
    Poly({{0,1,1},{0,0,1},{1,0,1},{1,1,1}})  //North
};

constexpr float EPSILON = 1e-5f;

void GeoUtils::rotatePoint(Vec3f& p, const Vec3f& center, const Vec3f& xyzRotRad) {
    glm::vec3 c(center.x, center.y, center.z);
    glm::mat4 rotation = glm::rotate(xyzRotRad.z, glm::vec3(0, 0, 1))*
                         glm::rotate(xyzRotRad.y, glm::vec3(0, 1, 0))*
                         glm::rotate(xyzRotRad.x, glm::vec3(1, 0, 0));
    glm::mat4 transform = glm::translate(c)*rotation*glm::translate(-c);
    glm::vec4 rotated = transform*glm::vec4(p.x, p.y, p.z, 1.0f);
    p = Vec3f(rotated.x, rotated.y, rotated.z);
}

bool GeoUtils::isPolyWithinPlane(const Poly& poly, const Vec3f& planePoint, const Vec3f& planeNormal) {
    for(int i = 0; i<poly.vs(); i++) {
        Vec3f v = poly.v(i).pos;
        float dist = (v-planePoint).dot(planeNormal);
        if(std::abs(dist)>EPSILON)
            return false;
    }
    return true;
}

bool GeoUtils::isPointInConvexPoly2D(const std::vector<Vec2f>& poly2D, const Vec2f& p) {
    bool sign = false;
    bool signSet = false;
    for(size_t i = 0; i<poly2D.size(); i++) {
        Vec2f a = poly2D[i];
        Vec2f b = poly2D[(i+1)%poly2D.size()];
        Vec2f edge = b-a;
        Vec2f toP = p-a;
        float crossZ = edge.x*toP.y - edge.y*toP.x;

        if(std::fabs(crossZ)<EPSILON) continue;

        bool currentSign = crossZ>0;
        if(!signSet) {
            sign = currentSign;
            signSet = true;
        } else if(currentSign!=sign) {
            return false;
        }
    }
    return true;
}

bool GeoUtils::isPolyWithinPoly(const Poly& inner, const Poly& outer) {
    //1. Compute outer plane
    Vec3f planePoint = outer.v(0).pos;
    Vec3f planeNormal = (outer.v(1).pos-outer.v(0).pos).cross(outer.v(2).pos-outer.v(0).pos).normalized();

    //2. Check inner is in same plane
    if(!isPolyWithinPlane(inner, planePoint, planeNormal))
        return false;

    //3. Build plane basis
    Vec3f u = (outer.v(1).pos-outer.v(0).pos).normalized();
    Vec3f v = planeNormal.cross(u).normalized();

    //4. Project outer vertices to 2D
    std::vector<Vec2f> outer2D;
    outer2D.reserve(outer.vs());
    for(int i = 0; i < outer.vs(); i++) {
        Vec3f vert = outer.v(i).pos;
        outer2D.emplace_back((vert-planePoint).dot(u), (vert-planePoint).dot(v));
    }

    //5. Check each inner vertex
    for(int i = 0; i<inner.vs(); i++) {
        Vec3f vert = inner.v(i).pos;
        Vec2f proj((vert-planePoint).dot(u), (vert-planePoint).dot(v));
        if(!isPointInConvexPoly2D(outer2D, proj))
            return false;
    }

    return true;
}
