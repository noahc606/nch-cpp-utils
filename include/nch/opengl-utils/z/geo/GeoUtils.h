#pragma once
#include <nch/math-utils/vec2.h>
#include <nch/math-utils/vec3.h>
#include "Poly.h"
namespace nch { class GeoUtils {
public:
    static const std::vector<Poly> unitCube;

    static void rotatePoint(Vec3f& p, const Vec3f& center, const Vec3f& xyzRotRad);
    static bool isPolyWithinPlane(const Poly& poly, const Vec3f& planePoint, const Vec3f& planeNormal);
    static bool isPointInConvexPoly2D(const std::vector<Vec2f>& poly2D, const Vec2f& p);
    static bool isPolyWithinPoly(const Poly& inner, const Poly& outer);
private:
}; }
