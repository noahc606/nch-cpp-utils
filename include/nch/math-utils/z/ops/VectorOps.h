#pragma once
#include "Vec3.h"

namespace nch { template<typename T> class VectorOps {
public:
    /*
        planeP: A point inside the plane which defines the plane.
        planeN: A vector normal to the plane intersecting point planeP.
        line1: Point 1 of the line which may or may not intersect the plane.
        line2: Point 2 of the same line.

        Returns: The intersection of the line and plane.
    */
    static Vec3<T> intersectPlane(Vec3<T>& planeP, Vec3<T>& planeN, const Vec3<T>& line1, const Vec3<T>& line2)
    {
        planeN = planeN.normalized();
        double planeD = -(planeN.dot(planeP));
        double ad = line1.dot(planeN);
        double bd = line2.dot(planeN);
        
        double t = (-planeD-ad)/(bd-ad);
        Vec3<T> line1To2 = line2-line1;
        Vec3<T> lineToIntersect = line1To2*t;
        
        return (line1+lineToIntersect);
    }
}; }