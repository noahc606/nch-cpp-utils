#pragma once
#include <cmath>
#include <cstdint>
#include <sstream>
#include <tuple>
#ifdef GLM_ENABLE
#include <glm/glm.hpp>
#endif

namespace nch { template <typename T> class Vec3
{
public:
    Vec3(){}
    Vec3(T x, T y, T z) {
        Vec3::x = x;
        Vec3::y = y;
        Vec3::z = z;
    }
    Vec3(std::tuple<T, T, T> xyz) {
        Vec3::x = std::get<0>(xyz);
        Vec3::y = std::get<1>(xyz);
        Vec3::z = std::get<2>(xyz);
    }
    Vec3(T xyz) { x = xyz; y = xyz; z = xyz; }

#ifdef GLM_ENABLE
    Vec3(const glm::ivec3& v)   { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); }
    Vec3(const glm::i64vec3& v) { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); }
    Vec3(const glm::fvec3& v)   { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); }
    Vec3(const glm::dvec3& v)   { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); }
#endif

    /** Getters **/
    //Functions
    T length2() const { return x*x+y*y+z*z; }
    T length() const { return std::sqrt(length2()); }
    std::tuple<T, T, T> tuple() const { return std::make_tuple(x, y, z); }
    T distanceTo(const Vec3<T>& v) const {
        return std::sqrt( (v.x-x)*(v.x-x) + (v.y-y)*(v.y-y) + (v.z-z)*(v.z-z) );
    }
    Vec3<T> getMidpoint(const Vec3<T>& v) const {
        return ((*this)+v)*0.5f;
    }
    Vec3<float> toFloat() const { return Vec3<float>(x, y, z); }
    Vec3<double> toDouble() const { return Vec3<double>(x, y, z); }
    Vec3<int64_t> toInt64() const { return Vec3<int64_t>(static_cast<int64_t>(x), static_cast<int64_t>(y), static_cast<int64_t>(z)); }
    Vec3<uint64_t> toUint64() const { return Vec3<uint64_t>(static_cast<uint64_t>(x), static_cast<uint64_t>(y), static_cast<uint64_t>(z)); }
    std::string toString() const { std::stringstream ss; ss << "(" << x << ", " << y << ", " << z << ")"; return ss.str(); }
    std::string toArrayString() const { std::stringstream ss; ss << "[" << x << "," << y << "," << z << "]"; return ss.str(); }

#ifdef GLM_ENABLE
    operator glm::dvec3() const { return glm::dvec3(x, y, z); }
    operator glm::fvec3() const { return glm::fvec3(x, y, z); }
    operator glm::ivec3() const { return glm::ivec3(x, y, z); }
    operator glm::i64vec3() const { return glm::i64vec3(x, y, z); }
#endif

    /** Operations **/
    //Basic operations
    Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }                           //Negate
    Vec3<T> operator+(const Vec3<T>& v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }  //Add vector to vector (translation)
    Vec3<T> operator-(const Vec3<T>& v) const { return (*this)+(-v); }                  //Subtract vector from vector (translation)
    Vec3<T> operator*(T r) const { return Vec3<T>(x*r, y*r, z*r); }                     //Scaling
    Vec3<T> operator/(T r) const { return Vec3<T>(x/r, y/r, z/r); }                     //Scaling
    //Multiplication w/ other vectors
    Vec3<T> operator*(const Vec3<T>& v) const { return Vec3<T>(x*v.x, y*v.y, z*v.z); }  //Not dot product! Returns "stretched" vector
    T dot(const Vec3<T>& v) const { return x*v.x+y*v.y+z*v.z; }
    Vec3<T> cross(const Vec3<T>& v) const { return Vec3<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x); }
    Vec3<T> normalized() {
        Vec3<T> res = (*this);
        T len2 = res.length2();
        if(len2>0) {
            double invLen = 1/std::sqrt(len2);
            res *= invLen;
        }
        return res;
    }
    
    //Comparison
    bool operator==(Vec3<T> v) const { return v.x==x && v.y==y && v.z==z; };
    bool operator!=(Vec3<T> v) const { return !(v==(*this)); };

    /** Mutators **/
    Vec3<T>& operator+=(const Vec3<T>& v) { x+=v.x, y+=v.y, z+=v.z; return *this; }     //Add-set
    Vec3<T>& operator-=(const Vec3<T>& v) { x-=v.x, y-=v.y, z-=v.z; return *this; }     //Add-set
    Vec3<T>& operator*=(const T& r) { x*=r, y*=r, z*=r; return *this; }                 //Scale-set
    Vec3<T>& operator=(const Vec3<T>& v) { x = v.x; y = v.y; z = v.z; return (*this); }

    T x = (T)0, y = (T)0, z = (T)0;
protected:

private:
};

typedef Vec3<int> Vec3i;
typedef Vec3<int64_t> Vec3i64;
typedef Vec3<uint64_t> Vec3u64;
typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;
}