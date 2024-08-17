#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "Color.h"

class NCH_Text {
public:

    ~NCH_Text();
    void init(SDL_Renderer* rend, TTF_Font* font, bool darkenBackground);
    void draw(int x, int y);
    
    double getWidth(); double getHeight();
    void setScale(double scale);
    void setText(std::u16string text);
    void setText(std::string text);
    void setWrapLength(int wl);
    void setDarkBackground(bool db);
private:
    SDL_Renderer* rend = nullptr;
    SDL_Texture* txtTex = nullptr;
    bool initted = false;
    bool darkenBackground = false;

    double scale = 1;
    double width = 0;
    double height = 0;
    std::u16string text = u"Unset";
    TTF_Font* font = nullptr;
    NCH_Color textColor = NCH_Color(255, 255, 255);
    int wrapLength = 9999;

    void updateTextTexture();
};