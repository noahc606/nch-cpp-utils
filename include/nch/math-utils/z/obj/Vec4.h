#pragma once
#include <stdexcept>
#include "Vec3.h"

//Single column of 4x4 matrix (Vec4 = column vector)
namespace nch { template <typename T> class Vec4 {
public:
    Vec4() { v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 1; }
    Vec4(T x, T y, T z, T w) { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }
    Vec4(Vec3<T> vec3) { v[0] = vec3.x; v[1] = vec3.y; v[2] = vec3.z; v[3] = 1; }

#ifdef GLM_ENABLE
    Vec4(const glm::ivec4& g) { v[0] = static_cast<T>(g.x); v[1] = static_cast<T>(g.y); v[2] = static_cast<T>(g.z); v[3] = static_cast<T>(g.w); }
    Vec4(const glm::fvec4& g) { v[0] = static_cast<T>(g.x); v[1] = static_cast<T>(g.y); v[2] = static_cast<T>(g.z); v[3] = static_cast<T>(g.w); }
    Vec4(const glm::dvec4& g) { v[0] = static_cast<T>(g.x); v[1] = static_cast<T>(g.y); v[2] = static_cast<T>(g.z); v[3] = static_cast<T>(g.w); }
    operator glm::dvec4() const { return glm::dvec4(v[0], v[1], v[2], v[3]); }
    operator glm::fvec4() const { return glm::fvec4(v[0], v[1], v[2], v[3]); }
    operator glm::ivec4() const { return glm::ivec4(v[0], v[1], v[2], v[3]); }
#endif

    //Bounds-checked, but the index is a literal at every call site, so the check folds away at compile time.
    T& operator[](int index) { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }
    T operator[](int index) const { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }

    Vec3<T> vec3() const { return Vec3<T>(v[0], v[1], v[2]); }
    std::string toString() const { std::stringstream ss; ss << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")"; return ss.str(); }

private:
    T v[4];
};

typedef Vec4<int> Vec4i;
typedef Vec4<int64_t> Vec4i64;
typedef Vec4<uint64_t> Vec4u64;
typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;
}