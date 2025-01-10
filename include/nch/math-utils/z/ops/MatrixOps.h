#pragma once
#include "Mat4x4.h"
#include "Vec3.h"

namespace nch { template<typename T> class MatrixOps {
public:
    /*
        Multiply 4D vector 'a' by the 4x4 matrix 'b'
    */
    static Vec4<T> multiply(const Vec4<T>& a, const Mat4x4<T>& b)
    {
        Vec4<T> res;
        res[0] = a[0]*b[0][0] + a[1]*b[1][0] + a[2]*b[2][0] + a[3]*b[3][0];
        res[1] = a[0]*b[0][1] + a[1]*b[1][1] + a[2]*b[2][1] + a[3]*b[3][1];
        res[2] = a[0]*b[0][2] + a[1]*b[1][2] + a[2]*b[2][2] + a[3]*b[3][2];
        res[3] = a[0]*b[0][3] + a[1]*b[1][3] + a[2]*b[2][3] + a[3]*b[3][3];
        return res;
    }
    static Vec3<T> multiply4d(const Vec3<T>& a, const Mat4x4<T>& b)
    {
        Vec4<T> a4 = a;
        return multiply(a4, b).vec3();
    }
    /*
        Matrix multiplication of two 4x4's
    */
    static Mat4x4<T> multiply(const Mat4x4<T>& a, const Mat4x4<T>& b)
    {
        Mat4x4<T> res;
        for(int r = 0; r<4; r++)
            for(int c = 0; c<4; c++)
                res[r][c] =
                    a[r][0]*b[0][c] +
                    a[r][1]*b[1][c] +
                    a[r][2]*b[2][c] +
                    a[r][3]*b[3][c];
        return res;
    }
    /*
        | 1 0 0 0 |
        | 0 1 0 0 |
        | 0 0 1 0 |
        | 0 0 0 1 |
    */
    static Mat4x4<T> getIdentityMatrix()
    {
		Mat4x4<T> res;
		res[0][0] = 1.; res[1][1] = 1.; res[2][2] = 1.; res[3][3] = 1.;
		return res;
    }
    /*
        | 1 0 0 x |
        | 0 1 0 y |
        | 0 0 1 z |
        | 0 0 0 1 |
    */
    static Mat4x4<T> getTranslationMatrix(const Vec3<T>& txyz)
    {
		Mat4x4<T> res = getIdentityMatrix();
		res[3][0] = txyz.x; res[3][1] = txyz.y; res[3][2] = txyz.z;
		return res;
    }
    /*
        | a 0 0 0 |
        | 0 b 0 0 |
        | 0 0 c 1 |
        | 0 0 d 0 |
        
        a: aspectRation*fovRad
        b: fovRad
        c: far/(far-near)
        d: (-far*near)/(far-near)
    */
    static Mat4x4<T> getProjectionMatrix()
    {
        //Matrix taken from https://www.youtube.com/watch?v=ih20l3pJoeU
        double near = 0.1;                                  //Near plane (distance from camera/eyes to screen)
        double far = 1000.0;                                //Far plane
        double fov = 90.0;                                  //Field of View
        double aspectRatio = 1.0;                           //Screen aspect ratio
        double fovRad = 1.0/std::tan( fov*0.5/180.0*M_PI ); //modified fov value, in radians

        Mat4x4<T> res;
        res[0][0] = aspectRatio*fovRad;
        res[1][1] = fovRad;
        res[2][2] = far/(far-near);
        res[2][3] = 1.0;
        res[3][2] = (-far*near)/(far-near);
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
    
    
    static Mat4x4<T> getPointAtMatrix(const Vec3<T>& pos, const Vec3<T>& target, const Vec3<T>& up)
    {
        //Resulting forward (rf)
        Vec3<T> rf = target-pos;
        rf = rf.normalized();
        //Resulting up (ru)
        Vec3<T> a = rf*(up.dot(rf));
        Vec3<T> ru = up-a;
        ru = ru.normalized();
        //Resulting right (rr)
        Vec3<T> rr = ru.cross(rf);
    
        Mat4x4<T> res;
		res[0][0] = rr.x;	res[0][1] = rr.y;	res[0][2] = rr.z;	res[0][3] = 0.;
		res[1][0] = ru.x;   res[1][1] = ru.y;   res[1][2] = ru.z;   res[1][3] = 0.;
		res[2][0] = rf.x;	res[2][1] = rf.y;	res[2][2] = rf.z;	res[2][3] = 0.;
		res[3][0] = pos.x;  res[3][1] = pos.y;  res[3][2] = pos.z;  res[3][3] = 1.;
        return res;
    }

    static Mat4x4<T> invSpecialMatrix(const Mat4x4<T>& m)
    {
		Mat4x4<T> res;
		res[0][0] = m[0][0];    res[0][1] = m[1][0];    res[0][2] = m[2][0];    res[0][3] = 0.;
		res[1][0] = m[0][1];    res[1][1] = m[1][1];    res[1][2] = m[2][1];    res[1][3] = 0.;
		res[2][0] = m[0][2];    res[2][1] = m[1][2];    res[2][2] = m[2][2];    res[2][3] = 0.;
        
        res[3][0] = -(m[3][0]*res[0][0] + m[3][1]*res[1][0] + m[3][2]*res[2][0]);
		res[3][1] = -(m[3][0]*res[0][1] + m[3][1]*res[1][1] + m[3][2]*res[2][1]);
		res[3][2] = -(m[3][0]*res[0][2] + m[3][1]*res[1][2] + m[3][2]*res[2][2]);
		res[3][3] = 1.;
		return res;
    }

private:

}; }