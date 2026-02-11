#include "TexUtils.h"
#include <stdexcept>

using namespace nch;

Color TexUtils::getPixelColor(void* pixels, SDL_PixelFormat* pxFmt, int pitch, int x, int y)
{
    //Ptr to pixel @ (x, y)
	uint8_t* pPixel = (uint8_t*)pixels + y*pitch+x*pxFmt->BytesPerPixel;
    //Get pixel as pixel data
	uint32_t pixelData = *(uint32_t*)pPixel;

	//Return final color
	Color res;
    Uint8 r = 0, g = 0, b = 0, a = 0;
	SDL_GetRGBA(pixelData, pxFmt, &r, &g, &b, &a);
	return Color(r, g, b, a);
}
Color TexUtils::getPixelColor(SDL_Surface* pSurface, int x, int y)
{
    if(x<0 || x>=pSurface->w || y<0 || y>=pSurface->h) {
        throw std::out_of_range("Specified pixel (x, y) is outside of the surface.");
    }

	//Bytes per pixel
	uint8_t bpp = pSurface->format->BytesPerPixel;
    //Ptr to pixel @ (x, y)
	uint8_t* pPixel = (uint8_t*)pSurface->pixels + y*pSurface->pitch+x*bpp;
    //Get pixel as pixel data
	uint32_t pixelData = *(uint32_t*)pPixel;

	//Return final color
	Color res;
    Uint8 r = 0, g = 0, b = 0, a = 0;
	SDL_GetRGBA(pixelData, pSurface->format, &r, &g, &b, &a);
	return Color(r, g, b, a);
}
void TexUtils::setPixelColor(void* pixels, int pitch, uint8_t bpp, int x, int y, uint32_t rgba)
{
    uint32_t* const targetPixel = (uint32_t*) ((uint8_t*)pixels+y*pitch+x*bpp);
    *targetPixel = rgba;
}
void TexUtils::setPixelColor(SDL_Surface* pSurface, int x, int y, uint32_t rgba)
{
    if(x<0 || x>=pSurface->w || y<0 || y>=pSurface->h) {
        throw std::out_of_range("Specified pixel (x, y) is outside of the surface.");
    }
    setPixelColor(pSurface->pixels, pSurface->pitch, pSurface->format->BytesPerPixel, x, y, rgba);
}

void TexUtils::clearTexture(GLSDL_Renderer* rend, GLSDL_Texture*& tex)
{
    //Save old render target, draw blend mode, and draw color
    GLSDL_Texture* oldRTarget = GLSDL_GetRenderTarget(rend);
    SDL_BlendMode oldRDBlendMode;   GLSDL_GetRenderDrawBlendMode(rend, &oldRDBlendMode);
    uint8_t oRDR, oRDG, oRDB, oRDA; GLSDL_GetRenderDrawColor(rend, &oRDR, &oRDG, &oRDB, &oRDA);

    //Clear the texture
    GLSDL_SetRenderTarget(rend, tex);                       //Set render target to tex we are clearing
    GLSDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_NONE); //Set blend mode to SDL_BLENDMODE_NONE to replace all pixels with transparency
    GLSDL_SetRenderDrawColor(rend, 0, 0, 0, 0);             //Set render draw color to invisible
    GLSDL_RenderFillRect(rend, NULL);                       //Fill tex with transparency

    //Restore old render target, draw blend mode, and draw color
    GLSDL_SetRenderTarget(rend, oldRTarget);
    GLSDL_SetRenderDrawBlendMode(rend, oldRDBlendMode);
    GLSDL_SetRenderDrawColor(rend, oRDR, oRDG, oRDB, oRDA);
}

/*
    Render a bordered filled rectangle (with foreground color==RenderDrawColor) to the render target.
    The border's size is clamped to 1 if it is less than 1.
    If border color NOT specified: it becomes the opposite color (255-r,g,b) of the current RenderDrawColor.
*/
void TexUtils::renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize, nch::Color borderColor)
{
    nch::Color bc = borderColor;

    uint8_t rr, rg, rb, ra;
    SDL_GetRenderDrawColor(rend, &rr, &rg, &rb, &ra);
    nch::Color fc = nch::Color(rr, rg, rb, ra);

    //Fill "background" rectangle (includes the outline and has the border color)
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, bc.r, bc.g, bc.b, bc.a);
    SDL_RenderFillRect(rend, r);

    //Fill the "inner" rectangle
    if(borderSize<1) borderSize = 1;
    SDL_Rect r2 = *r;
    r2.x += borderSize; r2.y += borderSize;
    r2.w -= 2*borderSize; r2.h -= 2*borderSize;
    SDL_SetRenderDrawColor(rend, fc.r, fc.g, fc.b, fc.a);
    SDL_RenderFillRect(rend, &r2);
}

void TexUtils::renderFillBorderedRect(SDL_Renderer* rend, SDL_Rect* r, int borderSize)
{
    uint8_t rr, rg, rb, ra;
    SDL_GetRenderDrawColor(rend, &rr, &rg, &rb, &ra);
    renderFillBorderedRect(rend, r, borderSize, nch::Color(255-rr, 255-rg, 255-rb));
}

void TexUtils::renderFillTri(SDL_Renderer* rend, int x1, int y1, int x2, int y2, int x3, int y3)
{
    SDL_Point p1; p1.x = x1; p1.y = y1;
    SDL_Point p2; p2.x = x2; p2.y = y2;
    SDL_Point p3; p3.x = x3; p3.y = y3;
    renderFillTri(rend, p1, p2, p3);
}