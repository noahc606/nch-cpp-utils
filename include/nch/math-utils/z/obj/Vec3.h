#pragma once
#include <cmath>
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

    /* Getters */
    //Functions
    T length2() { return x*x+y*y+z*z; }
    T length() { return std::sqrt(length2()); }
    std::tuple<T, T, T> tuple() { return std::make_tuple(x, y, z); }
    //Operations
    Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }                           //Negate
    Vec3<T> operator+(const Vec3<T>& v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }  //Add vector to vector
    Vec3<T> operator-(const Vec3<T>& v) const { return (*this)+(-v); }                  //Subtract vector from vector
    Vec3<T> operator*(T r) const { return Vec3<T>(x*r, y*r, z*r); }                     //Scaling
    Vec3<T> operator/(T r) const { return Vec3<T>(x/r, y/r, z/r); }                     //Scaling
    Vec3<T> operator*(const Vec3<T>& v) const { return Vec3<T>(x*v.x, y*v.y, z*v.z); }  //Not dot product! returns vector
    T dot(const Vec3<T>& v) const { return x*v.x+y*v.y+z*v.z; }
    //Comparison
    bool operator==(Vec3<T> v) const { return v.x==x && v.y==y && v.z==z; };
    bool operator!=(Vec3<T> v) const { return !(v==(*this)); };

    /* Mutators */
    Vec3<T>& operator+=(const Vec3<T>& v) { x+=v.x, y+=v.y, z+=v.z; return *this; }     //Add-set
    Vec3<T>& operator*=(const T& r) { x*=r, y*=r, z*=r; return *this; }                 //Scale-set
    Vec3<T>& normalize() {
        T len2 = length2();
        if(len2>0) {
            double invLen = 1/std::sqrt(len2);
            (*this) *= invLen;
        }
        return *this;
    }
    T distanceTo(const Vec3<T>& v) {
        return std::sqrt( (v.x-x)*(v.x-x) + (v.y-y)*(v.y-y) + (v.z-z)*(v.z-z) );
    }

    T x;
    T y;
    T z;
protected:

private:
}; }
