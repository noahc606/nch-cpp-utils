#pragma once
#include <glm/glm.hpp>
#include "Poly.h"
class GeoUtils {
public:
    static const std::vector<Poly> unitCube;

    static void rotatePoint(glm::vec3& p, const glm::vec3& center, const glm::vec3& xyzRot);
    static bool isPolyWithinPlane(const Poly& poly, const glm::vec3& planePoint, const glm::vec3& planeNormal);
    static bool isPointInConvexPoly2D(const std::vector<glm::vec2>& poly2D, const glm::vec2& p);
    static bool isPolyWithinPoly(const Poly& inner, const Poly& outer);
private:
};
