#include "Text.h"
#include <codecvt>
#include <iostream>
#include <locale>
using namespace nch;

Text::Text(){}
Text::~Text() { destroy(); }

void Text::init(SDL_Renderer* rend, TTF_Font* font, bool darkenBackground)
{
    if(initted) {
        destroy();
    }
    initted = true;
    
    //Set renderer and font
    Text::rend = rend;
    Text::font = font;
    Text::darkenBackground = darkenBackground;
}
void Text::init(SDL_Renderer* rend, TTF_Font* font) { init(rend, font, false); }

void Text::destroy()
{
    if(txtTex!=nullptr) {
        SDL_DestroyTexture(txtTex);
    }
    txtTex = nullptr;
}

void Text::draw(int x, int y)
{
    if(txtTex==nullptr) return;

    //Draw text
    SDL_Rect dst;
    dst.x = x; dst.y = y;
    dst.w = width*scale; dst.h = height*scale;
    
    if(darkenBackground) {
        SDL_SetRenderDrawColor(rend, 255-textColor.r, 255-textColor.g, 255-textColor.b, 100);
        SDL_RenderFillRect(rend, &dst);
    }

    SDL_SetTextureBlendMode(txtTex, SDL_BLENDMODE_BLEND);

    #if ( (SDL_MAJOR_VERSION>2) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION>0) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION==0 && SDL_PATCHLEVEL>=12))
        if(forceNearestScaling) {
            SDL_SetTextureScaleMode(txtTex, SDL_ScaleModeNearest);
        } else {
            SDL_SetTextureScaleMode(txtTex, SDL_ScaleModeBest);
        }
    #endif

    if(shadow.enabled) {
        dst.x += (shadow.dx*scale); dst.y += (shadow.dy*scale);

        if(shadow.customColor.a==0) {
            SDL_SetTextureColorMod(txtTex, 255-textColor.r, 255-textColor.g, 255-textColor.b);
        } else {
            SDL_SetTextureColorMod(txtTex, shadow.customColor.r, shadow.customColor.g, shadow.customColor.b);
        }
        
        SDL_SetTextureAlphaMod(txtTex, 255*shadow.fadeFactor);
        
        SDL_RenderCopy(rend, txtTex, NULL, &dst );

        dst.x -= (shadow.dx*scale); dst.y -= (shadow.dy*scale);
    }
    
    SDL_SetTextureColorMod(txtTex, textColor.r, textColor.g, textColor.b);
    SDL_SetTextureAlphaMod(txtTex, textColor.a);
    SDL_RenderCopy(rend, txtTex, NULL, &dst );
}

void Text::stream(SDL_Renderer* rend, TTF_Font* font, std::string text, Color c, int x, int y, double scale)
{
    int textWidth = 0;
    TTF_MeasureText(font, text.c_str(), 5000, &textWidth, NULL);
    int textHeight = TTF_FontHeight(font);
    
    SDL_Surface* txtSurf = TTF_RenderText_Solid(font, text.c_str(), {255, 255, 255});
    SDL_Texture* txtTex = SDL_CreateTextureFromSurface(rend, txtSurf);
    
    SDL_Rect txtRect; txtRect.x = x; txtRect.y = y; txtRect.w = textWidth*scale; txtRect.h = textHeight*scale;
    #if ( (SDL_MAJOR_VERSION>2) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION>0) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION==0 && SDL_PATCHLEVEL>=12))
        SDL_SetTextureScaleMode(txtTex, SDL_ScaleModeBest);
    #endif
    SDL_SetTextureColorMod(txtTex, c.r, c.g, c.b);
    SDL_RenderCopy(rend, txtTex, NULL, &txtRect);
    
    SDL_FreeSurface(txtSurf);
    SDL_DestroyTexture(txtTex);
}

double Text::getWidth() { return width*scale; }
double Text::getUnscaledWidth() { return width; }
double Text::getHeight() { return height*scale; }
double Text::getUnscaledHeight() { return height; }
std::u16string Text::getText() { return text; }

void Text::setScale(double scale)
{
    if(scale==Text::scale) return;

    Text::scale = scale;
    updateTextTexture();
}
void Text::forcedNearestScaling(bool fns)
{
    forceNearestScaling = fns;
}

void Text::setText(std::u16string text)
{
    if(text==Text::text) return;

    //Update string and update unscaled width/height
    Text::text = text;
    updateTextTexture();
}
void Text::setText(std::string text)
{
    //Convert string to unicode and set text
    std::string utf8_string = text;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string utf16_string = convert.from_bytes(utf8_string);
    const char16_t* unicodeText = utf16_string.c_str();

    setText(unicodeText);
}

void Text::setWrapLength(int wl)
{
    if(wl==Text::wrapLength) return;

    wrapLength = wl;
    updateTextTexture();
}

void Text::setDarkBackground(bool db) { darkenBackground = db; }
void Text::setTextColor(Color tc) { textColor = tc; }
void Text::setShadowing(bool hasShadow) { shadow.enabled = hasShadow; }
void Text::setShadowRelPos(int shadowDX, int shadowDY) { shadow.dx = shadowDX; shadow.dy = shadowDY; }
void Text::setShadowFadeFactor(float shadowFadeFactor) { shadow.fadeFactor = shadow.fadeFactor; }
void Text::removeShadowCustomColor() { shadow.customColor = nch::Color(0, 0, 0, 0); }
void Text::setShadowCustomColor(nch::Color shadowCustomColor)
{
    nch::Color scc = shadowCustomColor;
    shadow.customColor.r = scc.r;
    shadow.customColor.g = scc.g;
    shadow.customColor.b = scc.b;
    shadow.customColor.a = 255;
}

void Text::updateTextTexture()
{
    //Create surface representing the current text
    SDL_Surface* txtSurf = TTF_RenderUNICODE_Blended_Wrapped(font, (const Uint16*)text.c_str(), SDL_Color{255, 255, 255, 255}, wrapLength);
    if(txtSurf==NULL) {
        //Width will be zero so don't do anything
    } else {
        //Destroy the last texture if it exists and create a new one based on the txtSurf.
        if(txtTex!=nullptr) SDL_DestroyTexture(txtTex);
        txtTex = SDL_CreateTextureFromSurface(rend, txtSurf);

        //Set width and height, destroy the txtSurf.
        width = txtSurf->w;
        height = txtSurf->h;
    }

    SDL_FreeSurface(txtSurf);
}