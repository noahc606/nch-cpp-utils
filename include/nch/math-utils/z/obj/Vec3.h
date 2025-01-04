#pragma once
#include <cmath>
#include <cstdint>
#include <sstream>
#include <tuple>
#include "Mat4x4.h"

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
    
    /** Operations **/
    //Basic operations
    Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }                           //Negate
    Vec3<T> operator+(const Vec3<T>& v) const { return Vec3<T>(x+v.x, y+v.y, z+v.z); }  //Add vector to vector (translation)
    Vec3<T> operator-(const Vec3<T>& v) const { return (*this)+(-v); }                  //Subtract vector from vector (translation)
    Vec3<T> operator*(T r) const { return Vec3<T>(x*r, y*r, z*r); }                     //Scaling
    Vec3<T> operator/(T r) const { return Vec3<T>(x/r, y/r, z/r); }                     //Scaling
    //Multiplication w/ other vectors/matrices
    Vec3<T> operator*(const Vec3<T>& v) const { return Vec3<T>(x*v.x, y*v.y, z*v.z); }  //Not dot product! Returns "stretched" vector
    T dot(const Vec3<T>& v) const { return x*v.x+y*v.y+z*v.z; }
    /*
        Multiply vector 'a' (represented by this point) by the matrix 'b'
        Here, 'a' is treated as a 4D vector with w=1 and the original 3 coordinates (x,y,z) remaining the same.
        In the result, each of these 3 coordinates are divided by the w that we obtain.
        
        Returns: a 3D point which is a*b (a transformed by b).
    */
    Vec3<T> multiply4d(const nch::Mat4x4<double>& b) const
    {
        Vec3<T> res;
        res.x =     x*b[0][0] + y*b[1][0] + z*b[2][0] + b[3][0];
        res.y =     x*b[0][1] + y*b[1][1] + z*b[2][1] + b[3][1];
        res.z =     x*b[0][2] + y*b[1][2] + z*b[2][2] + b[3][2];
        double w =  x*b[0][3] + y*b[1][3] + z*b[2][3] + b[3][3];

        if( w!=0.0 ) {
            res.x /= w;
            res.y /= w;
            res.z /= w;
            return res;
        }
        
        return Vec3<T>();
    }
    
    //Comparison
    bool operator==(Vec3<T> v) const { return v.x==x && v.y==y && v.z==z; };
    bool operator!=(Vec3<T> v) const { return !(v==(*this)); };

    /** Mutators **/
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
    Vec3<T>& operator=(const Vec3<T>& v) { x = v.x; y = v.y; z = v.z; return (*this); }

    Vec3<double> toDouble() { return Vec3<double>(x, y, z); }
    Vec3<int64_t> toInt64() { return Vec3<int64_t>(x, y, z); }
    std::string toString() { std::stringstream ss; ss << "(" << x << ", " << y << ", " << z << ")"; return ss.str(); }
    std::string toArrayString() { std::stringstream ss; ss << "[" << x << "," << y << "," << z << "]"; return ss.str(); }

    T x = (T)0, y = (T)0, z = (T)0;
protected:

private:
}; }
