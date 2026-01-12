#include "GeoUtils.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

const std::vector<Poly> GeoUtils::unitCube = {
    Poly({{0,0,0},{0,0,1},{0,1,1},{0,1,0}}), //East
    Poly({{1,0,0},{1,1,0},{1,1,1},{1,0,1}}), //West
    Poly({{0,0,1},{0,0,0},{1,0,0},{1,0,1}}), //Down
    Poly({{1,1,1},{1,1,0},{0,1,0},{0,1,1}}), //Up
    Poly({{1,1,0},{1,0,0},{0,0,0},{0,1,0}}), //South
    Poly({{0,1,1},{0,0,1},{1,0,1},{1,1,1}})  //North
};

constexpr float EPSILON = 1e-5f;

void GeoUtils::rotatePoint(glm::vec3& p, const glm::vec3& center, const glm::vec3& xyzRot) {
    glm::mat4 rotation = glm::rotate(xyzRot.z, glm::vec3(0, 0, 1))*
                         glm::rotate(xyzRot.y, glm::vec3(0, 1, 0))*
                         glm::rotate(xyzRot.x, glm::vec3(1, 0, 0));
    glm::mat4 transform = glm::translate(center)*rotation*glm::translate(-center);
    glm::vec4 rotated = transform*glm::vec4(p, 1.0f);
    p = glm::vec3(rotated);
}

bool GeoUtils::isPolyWithinPlane(const Poly& poly, const glm::vec3& planePoint, const glm::vec3& planeNormal) {
    for(int i = 0; i<poly.vs(); i++) {
        glm::vec3 v = poly.v(i).pos;
        float dist = glm::dot(v-planePoint, planeNormal);
        if(std::abs(dist)>EPSILON)
            return false;
    }
    return true;
}

bool GeoUtils::isPointInConvexPoly2D(const std::vector<glm::vec2>& poly2D, const glm::vec2& p) {
    bool sign = false;
    bool signSet = false;
    for(size_t i = 0; i<poly2D.size(); i++) {
        glm::vec2 a = poly2D[i];
        glm::vec2 b = poly2D[(i+1)%poly2D.size()];
        glm::vec2 edge = b-a;
        glm::vec2 toP = p-a;
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
    glm::vec3 planePoint = outer.v(0).pos;
    glm::vec3 planeNormal = glm::normalize(glm::cross(outer.v(1).pos - outer.v(0).pos, outer.v(2).pos - outer.v(0).pos));

    //2. Check inner is in same plane
    if(!isPolyWithinPlane(inner, planePoint, planeNormal))
        return false;

    //3. Build plane basis
    glm::vec3 u = glm::normalize(outer.v(1).pos-outer.v(0).pos);
    glm::vec3 v = glm::normalize(glm::cross(planeNormal, u));

    //4. Project outer vertices to 2D
    std::vector<glm::vec2> outer2D;
    outer2D.reserve(outer.vs());
    for(int i = 0; i < outer.vs(); i++) {
        glm::vec3 vert = outer.v(i).pos;
        outer2D.emplace_back(glm::dot(vert-planePoint, u), glm::dot(vert-planePoint, v));
    }

    //5. Check each inner vertex
    for(int i = 0; i<inner.vs(); i++) {
        glm::vec3 vert = inner.v(i).pos;
        glm::vec2 proj(glm::dot(vert-planePoint, u), glm::dot(vert-planePoint, v));
        if(!isPointInConvexPoly2D(outer2D, proj))
            return false;
    }

    return true;
}
