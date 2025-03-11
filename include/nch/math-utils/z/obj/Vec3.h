#pragma once
#include <cmath>
#include <cstdint>
#include <sstream>
#include <tuple>

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

    /** Getters **/
    //Functions
    T length2() { return x*x+y*y+z*z; }
    T length() { return std::sqrt(length2()); }
    std::tuple<T, T, T> tuple() { return std::make_tuple(x, y, z); }
    T distanceTo(const Vec3<T>& v) {
        return std::sqrt( (v.x-x)*(v.x-x) + (v.y-y)*(v.y-y) + (v.z-z)*(v.z-z) );
    }
    Vec3<double> toDouble() { return Vec3<double>(x, y, z); }
    Vec3<int64_t> toInt64() { return Vec3<int64_t>(x, y, z); }
    std::string toString() { std::stringstream ss; ss << "(" << x << ", " << y << ", " << z << ")"; return ss.str(); }
    std::string toArrayString() { std::stringstream ss; ss << "[" << x << "," << y << "," << z << "]"; return ss.str(); }
    
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