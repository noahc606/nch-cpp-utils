#pragma once
#include <algorithm>
#include <GLSDL/GLSDL.h>
#include <tuple>
#include "nch/cpp-utils/color.h"
#include "nch/sdl-utils/z/gfx/FrameBuffer.h"

namespace nch { class TexUtils {
public:
    typedef std::tuple<double, double, double> t_tuple3d;

    static nch::Color getPixelColor(void* pixels, SDL_PixelFormat* pxFmt, int pitch, int x, int y);
    static nch::Color getPixelColor(SDL_Surface* pSurface, int x, int y);
    static void setPixelColor(void* pixels, int pitch, uint8_t bpp, int x, int y, uint32_t rgba);
    static void setPixelColor(SDL_Surface* pSurface, int x, int y, uint32_t rgba);
    static void clearTexture(GLSDL_Renderer* rend, GLSDL_Texture*& tex);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize, nch::Color borderColor);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize);
    /*
        Render a filled triangle to the render target using many horizontal lines.
        - Works in SDL<2.0.18 (does not use SDL_RenderGeometry()).
        - You can use either SDL_Point or SDL_FPoint for T.
    */
    template<typename T> static void renderFillTri(SDL_Renderer* rend, T p0, T p1, T p2)
    {
        //Sort points such that p[i].y increases as i increases.
        if(p0.y>p1.y) { std::swap(p0, p1); }
        if(p0.y>p2.y) { std::swap(p0, p2); }
        if(p1.y>p2.y) { std::swap(p1, p2); }

        //All y's turned to int (limitation due to the nature of this fill algo)
        p0.y = (int)p0.y;
        p1.y = (int)p1.y;
        p2.y = (int)p2.y;

        //Get intersection of y=p1.y with the side p0p2. Store into 'temp'.
        float dX;
        if(p2.x-p0.x!=0) {
            float slope = (float)(p2.y-p0.y)/(float)(p2.x-p0.x);
            dX = (p1.y-p0.y)/slope;
        } else {
            dX = 0;
        }
        T temp; temp.x = p0.x+dX; temp.y = p1.y;

        //Iterate thru each y value and draw a line to fill part of the triangle
        T* op = &p0;  //Outer point - Starts out as p0 which is the top point.
        for(int y = p0.y; y<=p2.y; y++) {
            //When y==p1.y, drawing of the "top part" was just finished -> "outer point" switches to p2 which is the bottom point.
            if(y==p1.y) op = &p2;

            //Calculate the start and end X-pos of the line we want to draw.
            float frac = (y-op->y)/(float)(p1.y-op->y);
            float x1 = op->x+frac*(float)(p1.x-op->x);
            float x2 = op->x+frac*(float)(temp.x-op->x);

            //Draw line at height 'y' with appropriate x1 and x2
            SDL_RenderDrawLine(rend, x1, y, x2, y);
        }
    }

    static void renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3);

    /*
        Testing: tex is a 32x32 array of Color
    */
    template<typename T> static void renderTexturedTri(FrameBuffer& fb, uint32_t*& srcTex, T p1, T p2, T p3, t_tuple3d t1, t_tuple3d t2, t_tuple3d t3)
    {
        //Sort points such that p[i].y increases as i increases.
        if(p1.y>p2.y) { std::swap(p1, p2); std::swap(t1, t2); }
        if(p1.y>p3.y) { std::swap(p1, p3); std::swap(t1, t3); }
        if(p2.y>p3.y) { std::swap(p2, p3); std::swap(t2, t3); }
        double u1 = std::get<0>(t1), v1 = std::get<1>(t1), w1 = std::get<2>(t1);
        double u2 = std::get<0>(t2), v2 = std::get<1>(t2), w2 = std::get<2>(t2);
        double u3 = std::get<0>(t3), v3 = std::get<1>(t3), w3 = std::get<2>(t3);

        //All y's turned to int (limitation due to the nature of this fill algo)
        p1.y = (int)p1.y;
        p2.y = (int)p2.y;
        p3.y = (int)p3.y;

        int dx1 = p2.x-p1.x;    int dy1 = p2.y-p1.y;    
        int dx2 = p3.x-p1.x;    int dy2 = p3.y-p1.y;    
        double du1 = u2-u1;     double dv1 = v2-v1;     double dw1 = w2-w1;
        double du2 = u3-u1;     double dv2 = v3-v1;     double dw2 = w3-w1;

        double texU, texV, texW;

        double daxStep = 0; double dbxStep = 0;
        double du1Step = 0; double dv1Step = 0; double dw1Step = 0;
        double du2Step = 0; double dv2Step = 0; double dw2Step = 0;

        if(dy1) daxStep = dx1/(double)abs(dy1);
        if(dy2) dbxStep = dx2/(double)abs(dy2);
        if(dy1) { du1Step = du1/(double)abs(dy1); dv1Step = dv1/(double)abs(dy1); dw1Step = dw1/(double)abs(dy1); }
        if(dy2) { du2Step = du2/(double)abs(dy2); dv2Step = dv2/(double)abs(dy2); dw2Step = dw2/(double)abs(dy2); }

        //Draw triangle (iterate over y)
        T* op = &p1;
        t_tuple3d* ot = &t1;
        for(int iy = p1.y; iy<=p3.y; iy++) {
            if(iy==p2.y) {
                dy1 = p3.y-p2.y; dx1 = p3.x-p2.x;
                du1 = u3-u2; dv1 = v3-v2; dw1 = w3-w2;

                if (dy1) daxStep = dx1 / (double)abs(dy1);
                if (dy2) dbxStep = dx2 / (double)abs(dy2);

                du1Step = 0, dv1Step = 0;
                if (dy1) du1Step = du1 / (double)abs(dy1);
                if (dy1) dv1Step = dv1 / (double)abs(dy1);
                if (dy1) dw1Step = dw1 / (double)abs(dy1);

                op = &p2;
                ot = &t2;
            }
            int ax = op->x+(double)(iy-op->y)*daxStep;
            int bx = p1.x+(double)(iy-p1.y)*dbxStep;
            
            double texSU = std::get<0>(*ot)+(double)(iy-op->y)*du1Step;
            double texSV = std::get<1>(*ot)+(double)(iy-op->y)*dv1Step;
            double texSW = std::get<2>(*ot)+(double)(iy-op->y)*dw1Step;

            double texEU = u1+(double)(iy-p1.y)*du2Step;
            double texEV = v1+(double)(iy-p1.y)*dv2Step;
            double texEW = w1+(double)(iy-p1.y)*dw2Step;

            if(ax>bx) {
                std::swap(ax, bx);
                std::swap(texSU, texEU);
                std::swap(texSV, texEV);
                std::swap(texSW, texEW);
            }

            texU = texSU;
            texV = texSV;
            texW = texSW;

            double tStep = 1./((double)(bx-ax));
            double t = 0.;

            //Draw triangle (iterate over x)
            for(int ix = ax; ix<bx; ix++) {
                texU = (1.-t)*texSU+t*texEU;
                texV = (1.-t)*texSV+t*texEV;
                texW = (1.-t)*texSW+t*texEW;
                t += tStep;

                if(texW>=fb.getPixelDepth(ix, iy)) {
                    double iu = texU/texW;
                    double iv = texV/texW;
                    int tpx = iu*32.;  if(tpx>31) tpx = 31; if(tpx<0) tpx = 0;
                    int tpy = iv*32.;  if(tpy>31) tpy = 31; if(tpy<0) tpy = 0;
                    fb.setPixel(ix, iy, srcTex[32*tpy+tpx], texW);
                }
            }
        }
    }

private:
}; }