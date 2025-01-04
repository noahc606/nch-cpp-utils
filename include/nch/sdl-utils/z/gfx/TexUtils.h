#pragma once
#include <SDL2/SDL.h>
#include "nch/cpp-utils/color.h"

namespace nch { class TexUtils {
public:
    static void clearTexture(SDL_Renderer* rend, SDL_Texture*& tex);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize, nch::Color borderColor);
    static void renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize);
    /*
        Render a filled triangle to the render target using many horizontal lines.
        - Works in SDL<2.0.18 (does not use SDL_RenderGeometry()).
        - You can use either SDL_Point or SDL_FPoint for T.
    */
    template<typename T> static void renderFillTri(SDL_Renderer* rend, T p0, T p1, T p2)
    {
        //Sort points such that p[i].y increases as i increases. Use 'temp' to quickly swapsort.
        T temp;
        if(p0.y>p1.y) { temp = p0; p0 = p1; p1 = temp; }
        if(p0.y>p2.y) { temp = p0; p0 = p2; p2 = temp; }
        if(p1.y>p2.y) { temp = p1; p1 = p2; p2 = temp; }

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
        temp.x = p0.x+dX; temp.y = p1.y;

        //Draw the "top part": anything above y=p1.y within the tri
        T* op = &p0;  //Outer point - within the loop this is either the upmost or downmost point of the tri.
        for(int y = p0.y; y<=p2.y; y++) {
            if(y==p1.y) op = &p2;

            //Calculate the start and end X-pos of the line we want to draw.
            float frac = (y-op->y)/(float)(p1.y-op->y);
            float x1 = op->x+frac*(float)(p1.x-op->x);
            float x2 = op->x+frac*(float)(temp.x-op->x);

            //Draw line at height 'y'
            SDL_RenderDrawLine(rend, x1, y, x2, y);
        }
    }
    static void renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3);

private:
};}