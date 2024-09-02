#include "TexUtils.h"

using namespace nch;

void TexUtils::clearTexture(SDL_Renderer* rend, SDL_Texture*& tex)
{
    //Save old render target, draw blend mode, and draw color
    SDL_Texture* oldRTarget = SDL_GetRenderTarget(rend);
    SDL_BlendMode oldRDBlendMode;   SDL_GetRenderDrawBlendMode(rend, &oldRDBlendMode);
    uint8_t oRDR, oRDG, oRDB, oRDA; SDL_GetRenderDrawColor(rend, &oRDR, &oRDG, &oRDB, &oRDA);

    //Clear the texture
    SDL_SetRenderTarget(rend, tex);                         //Set render target to tex we are clearing
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE);   //Set blend mode to SDL_BLENDMODE_NONE to replace all pixels with transparency
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);               //Set render draw color to invisible
    SDL_RenderFillRect(rend, NULL);                         //Fill tex with transparency

    //Restore old render target, draw blend mode, and draw color
    SDL_SetRenderTarget(rend, oldRTarget);
    SDL_SetRenderDrawBlendMode(rend, oldRDBlendMode);
    SDL_SetRenderDrawColor(rend, oRDR, oRDG, oRDB, oRDA);
}

/*
    Render a bordered filled rectangle to the render target.
    The border's color is the opposite color of the current RenderDrawColor.
    The border's size is clamped to 1 if it is less than 1.
*/
void TexUtils::renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, float borderSize)
{
    //"Background" rectangle
    uint8_t rr, rg, rb, ra;
    SDL_GetRenderDrawColor(rend, &rr, &rg, &rb, &ra);
    SDL_SetRenderDrawColor(rend, 255-rr, 255-rg, 255-rb, ra);
    SDL_RenderFillRect(rend, r);

    //Primary rectangle
    if(borderSize<1) borderSize = 1;
    SDL_Rect r2 = *r;
    r2.x += borderSize; r2.y += 1*borderSize;
    r2.w -= 2*borderSize; r2.h -= 2*borderSize;
    SDL_SetRenderDrawColor(rend, rr, rg, rb, ra);
    SDL_RenderFillRect(rend, &r2);
}

/*
    Render a filled triangle to the render target using many horizontal lines
    
    Works in SDL<2.0.18 (does not use SDL_RenderGeometry())
*/
void TexUtils::renderFillTri(SDL_Renderer* rend, SDL_FPoint p0, SDL_FPoint p1, SDL_FPoint p2)
{
    //Sort points such that p[i].y increases as i increases. Use 'temp' to quickly swapsort.
    SDL_FPoint temp;
    if(p0.y>p1.y) { temp = p0; p0 = p1; p1 = temp; }
    if(p0.y>p2.y) { temp = p0; p0 = p2; p2 = temp; }
    if(p1.y>p2.y) { temp = p1; p1 = p2; p2 = temp; }

    //Get intersection of y=p1.y with the side p0p2. Store into 'temp'.
    float slope = (p2.y-p0.y)/(p2.x-p0.x);
    float dX = (p1.y-p0.y)/slope;
    temp.x = p0.x+dX; temp.y = p1.y;

    //Draw the "top part": anything above y=p1.y within the tri
    SDL_FPoint* op = &p0;  //Outer point - within the loop this is either the upmost or downmost point of the tri.
    for(int y = p0.y; y<=p2.y; y++) {
        if(y==p1.y) op = &p2;

        //Calculate the start and end X-pos of the line we want to draw.
        float frac = (y-op->y)/(p1.y-op->y);
        float x1 = op->x+frac*(p1.x-op->x);
        float x2 = op->x+frac*(temp.x-op->x);

        //Draw line at height 'y'
        SDL_RenderDrawLineF(rend, x1, y, x2, y);
    }
}

void TexUtils::renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3)
{
    SDL_FPoint p1; p1.x = x1; p1.y = y1;
    SDL_FPoint p2; p2.x = x2; p2.y = y2;
    SDL_FPoint p3; p3.x = x3; p3.y = y3;
    renderFillTri(rend, p1, p2, p3);
}