#include "Text.h"
#include <codecvt>
#include <iostream>
#include <locale>
using namespace nch;

Text::Text(){}
Text::~Text()
{
    if(txtTex!=nullptr) {
        SDL_DestroyTexture(txtTex);
    }
}

void Text::init(SDL_Renderer* rend, TTF_Font* font, bool darkenBackground)
{
    if(initted) return;
    initted = true;
    
    //Set renderer and font
    Text::rend = rend;
    Text::font = font;
    Text::darkenBackground = darkenBackground;
}
void Text::init(SDL_Renderer* rend, TTF_Font* font) { init(rend, font, false); }

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

    SDL_SetTextureAlphaMod(txtTex, textColor.a);
    SDL_SetTextureBlendMode(txtTex, SDL_BLENDMODE_BLEND);

    #if ( (SDL_MAJOR_VERSION>2) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION>0) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION==0 && SDL_PATCHLEVEL>=12))
        SDL_SetTextureScaleMode(txtTex, SDL_ScaleModeBest);
    #endif

    if(shadow) {
        dst.x += (4*scale); dst.y += (4*scale);
        SDL_SetTextureColorMod(txtTex, 255-textColor.r, 255-textColor.g, 255-textColor.b);
        SDL_RenderCopy(rend, txtTex, NULL, &dst );

        dst.x -= (4*scale); dst.y -= (4*scale);
    }
    
    SDL_SetTextureColorMod(txtTex, textColor.r, textColor.g, textColor.b);
    SDL_RenderCopy(rend, txtTex, NULL, &dst );
}

double Text::getWidth() { return width*scale; }
double Text::getHeight() { return height*scale; }

void Text::setScale(double scale)
{
    if(scale==Text::scale) return;

    Text::scale = scale;
    updateTextTexture();
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

void Text::updateTextTexture()
{
    //Create surface representing the current text
    SDL_Surface* txtSurf = TTF_RenderUNICODE_Blended_Wrapped(font, (const Uint16*)text.c_str(), SDL_Color{255, 255, 255, 255}, wrapLength);
    if(txtSurf==NULL) {
        printf("Error rendering font: %s\n", TTF_GetError());
    }

    //Destroy the last texture if it exists and create a new one based on the txtSurf.
    if(txtTex!=nullptr) SDL_DestroyTexture(txtTex);
    txtTex = SDL_CreateTextureFromSurface(rend, txtSurf);

    //Set width and height, destroy the txtSurf.
    width = txtSurf->w;
    height = txtSurf->h;
    SDL_FreeSurface(txtSurf);
}