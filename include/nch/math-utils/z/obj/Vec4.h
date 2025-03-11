#pragma once
#include "Vec3.h"

//Single column of 4x4 matrix (Vec4 = column vector)
namespace nch { template <typename T> class Vec4 {
public:
    Vec4() { v[0] = 0; v[1] = 0; v[2] = 0; v[3] = 1; }
    Vec4(Vec3<T> vec3) { v[0] = vec3.x; v[1] = vec3.y; v[2] = vec3.z; v[3] = 1; }
    Vec4(T* v) : v(v) {}
    T& operator[](int index) { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }
    T operator[](int index) const { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }
    
    Vec3<T> vec3() { return Vec3(v[0], v[1], v[2]); }

private:
    T v[4];
};

typedef Vec4<int> Vec4i;
typedef Vec4<int64_t> Vec4i64;
typedef Vec4<uint64_t> Vec4u64;
typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;
}