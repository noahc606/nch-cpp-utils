#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "Color.h"

namespace nch { class Text {
public:
    struct TextShadow {
        bool enabled = true;
        int dx = 4;
        int dy = 4;
        float fadeFactor = 0.8;
        nch::Color customColor = nch::Color(0, 0, 0, 0);
    };

    Text();
    ~Text();
    void init(SDL_Renderer* rend, TTF_Font* font, bool darkenBackground = false);
    void destroy();
    void draw(int x, int y);
    static void stream(SDL_Renderer* rend, TTF_Font* font, std::string text, const nch::Color& c, int x, int y, double scale);
    
    bool isInitialized();
    double getWidth(); double getHeight();
    double getUnscaledWidth(); double getUnscaledHeight();
    std::u16string getText();

    void setScale(double scale);
    void forcedNearestScaling(bool fns);
    void setText(std::u16string text);
    void setText(std::string text);
    void setWrapLength(int wl);
    void setDarkBackground(bool db);
    void setTextColor(Color tc);
    void setShadowing(bool hasShadow);
    void setShadowRelPos(int shadowDX, int shadowDY);
    void setShadowFadeFactor(float shadowFadeFactor);
    void removeShadowCustomColor();
    void setShadowCustomColor(nch::Color shadowCustomColor);
    void updateTextTexture();

private:
    SDL_Renderer* rend = nullptr;
    SDL_Texture* txtTex = nullptr;
    bool initted = false;
    bool darkenBackground = false;
    
    bool forceNearestScaling = false;
    TextShadow shadow;

    double scale = 1;
    double width = 0;
    double height = 0;
    std::u16string text = u"Unset";
    TTF_Font* font = nullptr;
    Color textColor = Color(255, 255, 255);
    int wrapLength = 9999;
};
}