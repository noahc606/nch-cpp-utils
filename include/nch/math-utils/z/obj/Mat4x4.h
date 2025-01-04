#pragma once
#include <sstream>
#include <cmath>

/*
    Implementation of a 4x4 matrix.
    
    Most code in this class was ported from code I wrote for an old team project:
    https://github.com/SledgeThatJackal/SkyGazer/blob/main/app/src/main/java/com/echo/skygazer/gfx/math/Matrix4d.java
*/

namespace nch { template <typename T> class Mat4x4
{
public:
    //Single column of 4x4 matrix (Vec4 = column vector)
    template <typename U> class Vec4 {
    public:
        Vec4(){}
        Vec4(U* v) : v(v) {}
        U& operator[](int index) { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }
        U operator[](int index) const { if(index<0 || index>3) { throw std::out_of_range("Element index for Vec4 out of range (must be in [0, 3])."); } return v[index]; }
        
    private:
        U v[4];
    };

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
        for(int x = 0; x<4; x++) {
            ss << "[ ";
            for(int y = 0; y<4; y++) {
                ss << mat[x][y] << " ";
            }
            ss << "]\n";
        }

        return ss.str();
    }
    /*
        Projection matrix:
        
        | a 0 0 0 |
        | 0 b 0 0 |
        | 0 0 c d |
        | 0 0 1 0 |
        
        a: aspectRation*fovRad
        b: fovRad
        c: far/(far-near)
        d: (-far*near)/(far-near)
    */
    static Mat4x4<T> getProjMatrix(int screenWidth, int screenHeight)
    {
        //Matrix taken from https://www.youtube.com/watch?v=ih20l3pJoeU
        double near = 0.1;                                              //Near plane (distance from camera/eyes to screen)
        double far = 1000.0;                                            //Far plane
        double fov = 90.0;                                              //Field of View
        double aspectRatio = (double)screenWidth/(double)screenHeight;  //Screen aspect ratio
        double fovRad = 1.0/std::tan( fov*0.5/180.0*M_PI );             //modified fov value, in radians

        Mat4x4<T> res;
        res[0][0] = aspectRatio*fovRad;
        res[1][1] = fovRad;
        res[2][2] = far/(far-near);
        res[3][2] = 1.0;
        res[2][3] = (-far*near)/(far-near);
        return res;
    }
    static Mat4x4<T> getXRotMatrix(double theta)
    {
        Mat4x4<T> res;
        res[0][0] = 1;                  res[3][3] = res[0][0];
        res[1][1] = std::cos(theta);    res[2][2] = res[1][1];
        res[1][2] = std::sin(theta);    res[2][1] =-res[1][2];
        return res;
    }
    static Mat4x4<T> getYRotMatrix(double theta)
    {
        Mat4x4<T> res;
        res[1][1] = 1;                  res[3][3] = res[1][1];
        res[0][0] = std::cos(theta);    res[2][2] = res[0][0];
        res[0][2] = std::sin(theta);    res[2][0] =-res[0][2];
        return res;
    }
    static Mat4x4<T> getZRotMatrix(double theta)
    {
        Mat4x4<T> res;
        res[2][2] = 1;                  res[3][3] = res[2][2];
        res[0][0] = std::cos(theta);    res[1][1] = res[0][0];
        res[0][1] = std::sin(theta);    res[1][0] =-res[0][1];
        return res;
    }
    
    /*
        Return a single column vector form the 4x4 'mat'rix according to 'index'.
        Thus "Mat4x4[1][3]" would get the element at column 2, row 4 within the 4x4 matrix.
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