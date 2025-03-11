#pragma once
#include <cmath>
#include <cstdint>
#include <sstream>

namespace nch { template <typename T> class Vec2
{
public:
    Vec2(){}
    Vec2(T x, T y) {
        Vec2::x = x;
        Vec2::y = y;
    }

    /** Getters **/
    //Functions
    T length2() { return x*x+y*y; }
    T length() { return std::sqrt(length2()); }
    T distanceTo(const Vec2<T>& v) { return std::sqrt( (v.x-x)*(v.x-x) + (v.y-y)*(v.y-y) ); }
    Vec2<int> toInt() { return Vec2<int>(x, y); }
    Vec2<double> toDouble() { return Vec2<double>(x, y); }
    Vec2<int64_t> toInt64() { return Vec2<int64_t>(x, y); }
    std::string toString() { std::stringstream ss; ss << "(" << x << ", " << y << ")"; return ss.str(); }
    std::string toArrayString() { std::stringstream ss; ss << "[" << x << "," << y << "]"; return ss.str(); }

    /** Operations **/
    //Basic operations
    Vec2<T> operator-() const { return Vec2<T>(-x, -y); }                       //Negate
    Vec2<T> operator+(const Vec2<T>& v) const { return Vec2<T>(x+v.x, y+v.y); } //Add vector to vector (translation)
    Vec2<T> operator-(const Vec2<T>& v) const { return (*this)+(-v); }          //Subtract vector from vector (translation)
    Vec2<T> operator*(T r) const { return Vec2<T>(x*r, y*r); }                  //Scaling
    Vec2<T> operator/(T r) const { return Vec2<T>(x/r, y/r); }                  //Scaling
    Vec2<T> normalized() {
        Vec2<T> res = (*this);
        T len2 = res.length2();
        if(len2>0) {
            double invLen = 1/std::sqrt(len2);
            res *= invLen;
        }
        return res;
    }

    //Comparison
    bool operator==(Vec2<T> v) const { return v.x==x && v.y==y; };
    bool operator!=(Vec2<T> v) const { return !(v==(*this)); };

    /** Mutators **/
    Vec2<T>& operator+=(const Vec2<T>& v) { x+=v.x, y+=v.y; return *this; }     //Add-set
    Vec2<T>& operator-=(const Vec2<T>& v) { x-=v.x, y-=v.y; return *this; }     //Add-set
    Vec2<T>& operator*=(const T& r) { x*=r, y*=r; return *this; }                 //Scale-set
    Vec2<T>& operator=(const Vec2<T>& v) { x = v.x; y = v.y; return (*this); }

    T x = (T)0, y = (T)0;
};

typedef Vec2<int> Vec2i;
typedef Vec2<int64_t> Vec2i64;
typedef Vec2<uint64_t> Vec2u64;
typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;
}