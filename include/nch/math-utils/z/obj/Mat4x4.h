#pragma once
#include <sstream>
#include <cmath>
#include "Vec4.h"

/*
    Implementation of a 4x4 matrix.
    
    Most code in this class was ported from code I wrote for an old team project:
    https://github.com/SledgeThatJackal/SkyGazer/blob/main/app/src/main/java/com/echo/skygazer/gfx/math/Matrix4d.java
*/

namespace nch { template <typename T> class Mat4x4
{
public:
    Mat4x4() { init(); }
    Mat4x4(const Mat4x4<T>& m) {
        if(mat==nullptr) init();
        (*this) = m;
    }
    ~Mat4x4() {
        if(mat!=nullptr) {
            delete[] mat;
        }
    }

    /* Getters */
    std::string toString()
    {
        std::stringstream ss;
        ss << "Mat4x4:\n";
        for(int r = 0; r<4; r++) {
            ss << "[ ";
            for(int c = 0; c<4; c++) {
                ss << mat[r][c] << " ";
            }
            ss << "]\n";
        }

        return ss.str();
    }

    /*
        Return a single row vector from the 4x4 'mat'rix according to 'index'.
        Thus "Mat4x4[1][3]" would get the element at row 2, col 4 within the 4x4 matrix.
        Another way to look at it: Mat4x4[1][3] points to element at x=3, y=1
    */
    Vec4<T>& operator[](int index) { if(index<0 || index>3) { throw std::out_of_range("Column index for Mat4x4 out of range (must be in [0, 3])."); } return mat[index]; }
    Vec4<T> operator[](int index) const { if(index<0 || index>3) { throw std::out_of_range("Column index for Mat4x4 out of range (must be in [0, 3])."); } return mat[index]; }

    /** Mutators **/
    Mat4x4<T>& operator=(const Mat4x4<T>& m)
    {
        for(int i = 0; i<4; i++)
            for(int j = 0; j<4; j++)
                mat[i][j] = m[i][j];
        return (*this);
    }

private:
    void init()
    {
        mat = new Vec4<T>[4];
        for(int i = 0; i<4; i++) {
            for(int j = 0; j<4; j++) {
                mat[i][j] = (T)0;
            }
        }
    }
    Vec4<T>* mat = nullptr;
}; }