#pragma once
#include <GLSDL/GLSDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdint>
#include <string>
#include <vector>
#include "Color.h"

namespace nch { class Text {
public:
    enum class Align { LEFT, CENTER, RIGHT };

    struct TextShadow {
        bool enabled = true;
        int dx = 4;
        int dy = 4;
        float fadeFactor = 0.8;
        nch::Color customColor = nch::Color(0, 0, 0, 0);
    };

    Text();
    Text(Text&& obj) noexcept;
    Text& operator=(const Text& obj);
    ~Text();
    void init(GLSDL_Renderer* rend, TTF_Font* font, bool darkenBackground = false);
    void destroy();
    void draw(int x, int y) const;
    void drawCentered(int x, int y, int w, int h) const;
    /**
     * @brief Like draw(), but honors the per-span colors and highlights parsed from the sentinel
     * markup. Falls back to draw() when the text carries no spans, so it is always safe to call.
     */
    void drawFormatted(int x, int y) const;
    void drawFormattedCentered(int x, int y, int w, int h) const;
    static void stream(GLSDL_Renderer* rend, TTF_Font* font, std::string text, const nch::Color& c, int x, int y, double scale);

    bool isInitialized() const;
    double getScale() const;
    double getWidth() const; double getHeight() const;
    double getUnscaledWidth() const; double getUnscaledHeight() const;
    std::u16string getText() const;
    std::u16string getPlainText() const;
    int getLineCount() const;
    int getLineHeight() const;
    int getTabWidth() const;
    //The drawn baseline-to-baseline distance: the effective line height plus lineSpacing.
    int getLinePitch() const;
    Align getAlign() const;
    char16_t getSentinel() const;
    GLSDL_Texture* getTexture() const;
    TTF_Font* getFont() const;

    bool setScale(double scale);
    void setGridlock(int px);
    void forcedNearestScaling(bool fns);
    bool setText(std::u16string text); bool setText(std::string text);
    void setWrapLength(int wl);
    /**
     * @brief Override the base distance between consecutive lines, before lineSpacing is added.
     * @param px The line height in unscaled pixels, or 0 to derive it from the font (the default).
     */
    void setLineHeight(int px);
    /**
     * @brief Set how far a '\t' advances. Each tab adds this outright rather than seeking the next
     * absolute stop, so leading tabs indent predictably under a proportional font.
     * @param px The advance in unscaled pixels, or 0 for four spaces' worth (the default).
     */
    void setTabWidth(int px);
    void setLineSpacing(int ls);
    void setMaxLines(int ml);
    void setEveryLineCentered(bool elc);
    void setAlign(Align align);
    /**
     * @brief Enable span markup, where '<sentinel>[...]' opens a span and '<sentinel>[]' restores
     * the default style. A doubled sentinel is a literal one. Spans do not nest: each one edits the
     * running style until the next reset. Attributes are '#rrggbb[aa]' shorthand for the foreground,
     * or 'fg='/'bg=' (aliases 'color='/'hl=') taking a hex value or 'none' to clear that attribute.
     * @param sentinel The marker character, or 0 to disable parsing entirely (the default).
     */
    void setSentinel(char16_t sentinel);
    void setDarkBackground(bool db);
    void setTextColor(Color tc);
    void setShadowing(bool hasShadow);
    void setShadowRelPos(int shadowDX, int shadowDY);
    void setShadowFadeFactor(float shadowFadeFactor);
    void removeShadowCustomColor();
    void setShadowCustomColor(nch::Color shadowCustomColor);
    void updateTextTexture();

private:
    struct Style {
        bool hasFg = false;
        bool hasBg = false;
        Color fg = Color(255, 255, 255);
        Color bg = Color(0, 0, 0);
        bool matches(const Style& o) const;
    };
    //One same-styled piece of one laid-out line, addressing its own pixels within the baked texture.
    //src.y doubles as the top of the enclosing line box, which is what a highlight fills.
    struct Run {
        SDL_Rect src = {0, 0, 0, 0};
        Style style;
    };
    //One same-styled piece of a line before it is baked, measured but not yet placed. A tab piece
    //carries no text at all - it is pure advance, and only exists so a span highlight spans the gap.
    struct LinePiece {
        uint16_t styleId = 0;
        std::u16string txt;
        int w = 0;
        bool isTab = false;
    };
    //A [start, start+len) slice of plainText occupying one line. Lines are contiguous slices, so a
    //style lookup is just an index into styleIds - no per-line string copies are kept around.
    struct LineRange {
        size_t start = 0;
        size_t len = 0;
        bool ellipsized = false;
    };

    static bool isLowSurrogate(char16_t c);
    //Both space and tab end a word, so either is somewhere a line may be broken.
    static bool isBreakSpace(char16_t c);
    static int measureWidth(TTF_Font* font, const std::u16string& txt, size_t start, size_t len, int tabAdvance);
    //How many code units of txt[start..start+len) fit within maxWidth, never splitting a surrogate pair.
    static size_t measureFitCount(TTF_Font* font, const std::u16string& txt, size_t start, size_t len, int maxWidth, int tabAdvance);
    static void applySpanAttribs(Style& style, const std::u16string& body);
    static uint16_t internStyle(std::vector<Style>& palette, const Style& style);
    static SDL_Surface* renderRunSurface(TTF_Font* font, const std::u16string& txt, size_t start, size_t len);

    //Stage 1: strip the sentinel markup off 'text', producing plainText + a parallel style index.
    void parseMarkup();
    //Stage 2: break plainText into lines, honoring explicit newlines, wrapLength and maxLines.
    void layoutLines(std::vector<LineRange>& lines) const;
    //Stage 3: blit every run onto one CPU surface, filling 'runs' as it goes. Caller uploads it.
    SDL_Surface* buildSurface(const std::vector<LineRange>& lines);
    void buildLinePieces(const LineRange& line, std::vector<LinePiece>& out) const;
    int getEffWrapLength() const;
    int getEffTabAdvance() const;
    //Shared by draw() and drawFormatted(): the shadow color for a run drawn in 'fg'.
    Color getShadowColorFor(const Color& fg) const;
    void drawRunPass(int bx, int by, bool shadowPass) const;

    GLSDL_Renderer* rend = nullptr;
    GLSDL_Texture* txtTex = nullptr;
    bool initted = false;
    bool darkenBackground = false;
    double width = 0;
    double height = 0;

    bool forceNearestScaling = false;
    TextShadow shadow;
    double scale = 1;
    int gridlock = 1;
    std::u16string text = u"";
    TTF_Font* font = nullptr;
    Color textColor = Color(255, 255, 255);
    int wrapLength = 9999;
    int lineHeight = 0;  //Base line-to-line distance; 0 derives it from TTF_FontHeight.
    int lineSpacing = 0; //Extra unscaled pixels added on top of the line height.
    int tabWidth = 0;    //Advance per tab; 0 derives it from four of the font's spaces.
    int maxLines = -1;
    Align align = Align::LEFT;
    char16_t sentinel = 0; //0 disables span parsing, so callers that never opt in keep literal text.

    //Bake products, all rebuilt by updateTextTexture().
    std::u16string plainText = u"";
    std::vector<uint16_t> styleIds; //Parallel to plainText; empty means every character is default.
    std::vector<Style> stylePalette;
    std::vector<Run> runs;
    int lineCount = 0;
    int linePitch = 0;
    int tabAdvance = 0;
    bool anyStyledRun = false;
};
}
