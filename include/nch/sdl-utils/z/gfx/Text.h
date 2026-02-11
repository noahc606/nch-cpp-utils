#pragma once
#include <GLSDL/GLSDL.h>
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
    void init(GLSDL_Renderer* rend, TTF_Font* font, bool darkenBackground = false);
    void destroy();
    void draw(int x, int y);
    void drawCentered(int x, int y, int w, int h);
    void drawRightAligned(int x, int y, int w, int h);
    static void stream(GLSDL_Renderer* rend, TTF_Font* font, std::string text, const nch::Color& c, int x, int y, double scale);

    bool isInitialized();
    double getScale();
    double getWidth(); double getHeight();
    double getUnscaledWidth(); double getUnscaledHeight();
    std::u16string getText();
    GLSDL_Texture* getTexture();

    bool setScale(double scale);
    void forcedNearestScaling(bool fns);
    bool setText(std::u16string text); bool setText(std::string text);
    void setWrapLength(int wl);
    void setMaxLines(int ml);
    void setEveryLineCentered(bool elc);
    void setDarkBackground(bool db);
    void setTextColor(Color tc);
    void setShadowing(bool hasShadow);
    void setShadowRelPos(int shadowDX, int shadowDY);
    void setShadowFadeFactor(float shadowFadeFactor);
    void removeShadowCustomColor();
    void setShadowCustomColor(nch::Color shadowCustomColor);
    void updateTextTexture();

    
private:
    static int measureTextWidth(TTF_Font* font, const std::u16string& text);
    static std::vector<std::pair<int, std::u16string>> getProcessedText(const std::u16string& text, TTF_Font* font, int maxWidth, int maxLines);

    GLSDL_Renderer* rend = nullptr;
    GLSDL_Texture* txtTex = nullptr;
    bool initted = false;
    bool darkenBackground = false;
    double width = 0;
    double height = 0;

    bool forceNearestScaling = false;
    TextShadow shadow;
    double scale = 1;
    std::u16string text = u"";
    TTF_Font* font = nullptr;
    Color textColor = Color(255, 255, 255);
    int wrapLength = 9999;
    int maxLines = -1;
    bool everyLineCentered = false;
};
}