#include "Text.h"
#include <codecvt>
#include <iostream>
#include <locale>

NCH_Text::~NCH_Text()
{
    if(txtTex!=nullptr) {
        SDL_DestroyTexture(txtTex);
    }
}

void NCH_Text::init(SDL_Renderer* rend, TTF_Font* font, bool darkenBackground)
{
    if(initted) return;
    initted = true;
    
    //Set renderer and font
    NCH_Text::rend = rend;
    NCH_Text::font = font;
    NCH_Text::darkenBackground = darkenBackground;
}

void NCH_Text::draw(int x, int y)
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

    SDL_SetRenderDrawColor(rend, textColor.r, textColor.g, textColor.b, 255);

    #if ( (SDL_MAJOR_VERSION>2) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION>0) || (SDL_MAJOR_VERSION==2 && SDL_MINOR_VERSION==0 && SDL_PATCHLEVEL>=12))
        SDL_SetTextureScaleMode(txtTex, SDL_ScaleModeBest);
    #endif
    SDL_RenderCopy(rend, txtTex, NULL, &dst );
}

double NCH_Text::getWidth() { return width*scale; }
double NCH_Text::getHeight() { return height*scale; }

void NCH_Text::setScale(double scale)
{
    if(scale==NCH_Text::scale) return;

    NCH_Text::scale = scale;
    updateTextTexture();
}
void NCH_Text::setText(std::u16string text)
{
    if(text==NCH_Text::text) return;

    //Update string and update unscaled width/height
    NCH_Text::text = text;
    updateTextTexture();
}
void NCH_Text::setText(std::string text)
{
    //Convert string to unicode and set text
    std::string utf8_string = text;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string utf16_string = convert.from_bytes(utf8_string);
    const char16_t* unicodeText = utf16_string.c_str();

    setText(unicodeText);
}

void NCH_Text::setWrapLength(int wl)
{
    if(wl==NCH_Text::wrapLength) return;

    wrapLength = wl;
    updateTextTexture();
}

void NCH_Text::setDarkBackground(bool db) { darkenBackground = db; }

void NCH_Text::updateTextTexture()
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