#include "Text.h"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <cctype>
#include <climits>
#include <codecvt>
#include <cstddef>
#include <locale>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <string>
using namespace nch;

static const std::u16string ELLIPSIS = u"...";

Text::Text() {
    initted = false;
    txtTex = nullptr;
}
Text::Text(Text&& obj) noexcept {
    rend = obj.rend;
    txtTex = obj.txtTex; obj.txtTex = nullptr;
    initted = obj.initted; obj.initted = false;
    darkenBackground = obj.darkenBackground;
    width = obj.width;
    height = obj.height;
    forceNearestScaling = obj.forceNearestScaling;
    shadow = obj.shadow;
    scale = obj.scale;
    gridlock = obj.gridlock;
    text = std::move(obj.text);
    font = obj.font;
    textColor = obj.textColor;
    wrapLength = obj.wrapLength;
    lineHeight = obj.lineHeight;
    lineSpacing = obj.lineSpacing;
    tabWidth = obj.tabWidth;
    maxLines = obj.maxLines;
    align = obj.align;
    sentinel = obj.sentinel;
    plainText = std::move(obj.plainText);
    styleIds = std::move(obj.styleIds);
    stylePalette = std::move(obj.stylePalette);
    runs = std::move(obj.runs);
    lineCount = obj.lineCount;
    linePitch = obj.linePitch;
    tabAdvance = obj.tabAdvance;
    anyStyledRun = obj.anyStyledRun;
}
Text& Text::operator=(const Text& obj)
{
    if(this == &obj) return *this;

    destroy();
    rend = obj.rend;
    initted = obj.initted;
    darkenBackground = obj.darkenBackground;
    forceNearestScaling = obj.forceNearestScaling;
    shadow = obj.shadow;
    scale = obj.scale;
    gridlock = obj.gridlock;
    text = obj.text;
    font = obj.font;
    textColor = obj.textColor;
    wrapLength = obj.wrapLength;
    lineHeight = obj.lineHeight;
    lineSpacing = obj.lineSpacing;
    tabWidth = obj.tabWidth;
    maxLines = obj.maxLines;
    align = obj.align;
    sentinel = obj.sentinel;

    updateTextTexture();
    return *this;
}
Text::~Text() { destroy(); }

void Text::init(GLSDL_Renderer* rend, TTF_Font* font, bool darkenBackground)
{
    if(initted) {
        destroy();
    }
    initted = true;

    Text::rend = rend;
    Text::font = font;
    Text::darkenBackground = darkenBackground;

    //A caller that set its text before init() would otherwise be left with nothing baked.
    updateTextTexture();
}

void Text::destroy()
{
    initted = false;
    if(txtTex!=nullptr) GLSDL_DestroyTexture(txtTex);
    txtTex = nullptr;
    runs.clear();
    anyStyledRun = false;
    lineCount = 0;
    width = 0;
    height = 0;
}

void Text::draw(int x, int y) const
{
    if(txtTex==nullptr) return;

    SDL_Rect dst;
    dst.x = x/gridlock*gridlock; dst.y = y/gridlock*gridlock;
    dst.w = width*scale; dst.h = height*scale;

    if(darkenBackground) {
        GLSDL_SetRenderDrawColor(rend, 255-textColor.r, 255-textColor.g, 255-textColor.b, 100);
        GLSDL_RenderFillRect(rend, &dst);
    }

    GLSDL_SetTextureBlendMode(txtTex, SDL_BLENDMODE_BLEND);

    if(shadow.enabled) {
        dst.x += (shadow.dx*scale); dst.y += (shadow.dy*scale);

        Color sc = getShadowColorFor(textColor);
        GLSDL_SetTextureColorMod(txtTex, sc.r, sc.g, sc.b);
        GLSDL_SetTextureAlphaMod(txtTex, 255*shadow.fadeFactor);
        GLSDL_RenderCopy(rend, txtTex, NULL, &dst);

        dst.x -= (shadow.dx*scale); dst.y -= (shadow.dy*scale);
    }

    GLSDL_SetTextureColorMod(txtTex, textColor.r, textColor.g, textColor.b);
    GLSDL_SetTextureAlphaMod(txtTex, textColor.a);
    GLSDL_RenderCopy(rend, txtTex, NULL, &dst);
}

void Text::drawCentered(int x, int y, int w, int h) const
{
    draw(x+w/2-(int)getWidth()/2, y+h/2-(int)getHeight()/2);
}

void Text::drawFormatted(int x, int y) const
{
    if(txtTex==nullptr) return;
    //Nothing carries a span, so the single-blit path renders the exact same pixels for less work.
    if(!anyStyledRun) { draw(x, y); return; }

    int bx = x/gridlock*gridlock;
    int by = y/gridlock*gridlock;

    if(darkenBackground) {
        SDL_Rect bg = {bx, by, (int)(width*scale), (int)(height*scale)};
        GLSDL_SetRenderDrawColor(rend, 255-textColor.r, 255-textColor.g, 255-textColor.b, 100);
        GLSDL_RenderFillRect(rend, &bg);
    }

    //Highlights fill the whole line pitch so stacked highlighted lines read as one unbroken block.
    GLSDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    for(const Run& r : runs) {
        if(!r.style.hasBg) continue;
        SDL_Rect hr;
        hr.x = bx+(int)(r.src.x*scale);
        hr.y = by+(int)(r.src.y*scale);
        hr.w = (int)(r.src.w*scale);
        hr.h = (int)(linePitch*scale);
        GLSDL_SetRenderDrawColor(rend, r.style.bg.r, r.style.bg.g, r.style.bg.b, r.style.bg.a);
        GLSDL_RenderFillRect(rend, &hr);
    }

    GLSDL_SetTextureBlendMode(txtTex, SDL_BLENDMODE_BLEND);
    if(shadow.enabled) drawRunPass(bx, by, true);
    drawRunPass(bx, by, false);
}

void Text::drawFormattedCentered(int x, int y, int w, int h) const
{
    drawFormatted(x+w/2-(int)getWidth()/2, y+h/2-(int)getHeight()/2);
}

void Text::stream(GLSDL_Renderer* rend, TTF_Font* font, std::string text, const Color& c, int x, int y, double scale)
{
    SDL_Surface* txtSurf = TTF_RenderText_Blended(font, text.c_str(), {255, 255, 255, 255});
    if(txtSurf==NULL) return;
    GLSDL_Texture* txtTex = GLSDL_CreateTextureFromSurface(rend, txtSurf);

    if(txtTex!=NULL) {
        SDL_Rect txtRect;
        txtRect.x = x; txtRect.y = y;
        txtRect.w = txtSurf->w*scale; txtRect.h = txtSurf->h*scale;
        GLSDL_SetTextureBlendMode(txtTex, SDL_BLENDMODE_BLEND);
        GLSDL_SetTextureColorMod(txtTex, c.r, c.g, c.b);
        GLSDL_SetTextureAlphaMod(txtTex, c.a);
        GLSDL_RenderCopy(rend, txtTex, NULL, &txtRect);
        GLSDL_DestroyTexture(txtTex);
    }

    SDL_FreeSurface(txtSurf);
}

bool Text::isInitialized() const { return initted; }
double Text::getScale() const { return scale; }
double Text::getWidth() const { return width*scale; }
double Text::getUnscaledWidth() const { return width; }
double Text::getHeight() const { return height*scale; }
double Text::getUnscaledHeight() const { return height; }
std::u16string Text::getText() const { return text; }
std::u16string Text::getPlainText() const { return plainText; }
int Text::getLineCount() const { return lineCount; }
int Text::getLineHeight() const { return lineHeight; }
int Text::getTabWidth() const { return tabWidth; }
int Text::getLinePitch() const { return linePitch; }
Text::Align Text::getAlign() const { return align; }
char16_t Text::getSentinel() const { return sentinel; }
GLSDL_Texture* Text::getTexture() const {
    if(!initted) return nullptr;
    return txtTex;
}
TTF_Font* Text::getFont() const { return font; }

bool Text::setScale(double scale)
{
    if(scale==Text::scale) return false;

    //Scale is applied to the destination rect at draw time, so the baked texture is untouched.
    Text::scale = scale;
    return true;
}
void Text::setGridlock(int px) { if(px<=0) { Log::warnv(__PRETTY_FUNCTION__, "setting to default of 1", "Gridlock must be a positive number"); px = 1; } gridlock = px; }
void Text::forcedNearestScaling(bool fns) { forceNearestScaling = fns; }

bool Text::setText(std::u16string text)
{
    if(text==Text::text) return false;

    Text::text = text;
    updateTextTexture();
    return true;
}
bool Text::setText(std::string text)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return setText(convert.from_bytes(text));
}

void Text::setWrapLength(int wl)
{
    if(wl==wrapLength) return;
    wrapLength = wl;
    updateTextTexture();
}
void Text::setLineHeight(int px)
{
    if(px<0) px = 0;
    if(px==lineHeight) return;
    lineHeight = px;
    updateTextTexture();
}
void Text::setTabWidth(int px)
{
    if(px<0) px = 0;
    if(px==tabWidth) return;
    tabWidth = px;
    updateTextTexture();
}
void Text::setLineSpacing(int ls)
{
    if(ls<0) ls = 0;
    if(ls==lineSpacing) return;
    lineSpacing = ls;
    updateTextTexture();
}
void Text::setMaxLines(int ml)
{
    if(ml<1) ml = -1;
    if(ml==maxLines) return;
    maxLines = ml;
    updateTextTexture();
}
void Text::setEveryLineCentered(bool elc)
{
    setAlign(elc ? Align::CENTER : Align::LEFT);
}
void Text::setAlign(Align align)
{
    if(align==Text::align) return;
    Text::align = align;
    updateTextTexture();
}
void Text::setSentinel(char16_t sentinel)
{
    if(sentinel==Text::sentinel) return;
    Text::sentinel = sentinel;
    updateTextTexture();
}

void Text::setDarkBackground(bool db) { darkenBackground = db; }
void Text::setTextColor(Color tc) { textColor = tc; }
void Text::setShadowing(bool hasShadow) { shadow.enabled = hasShadow; }
void Text::setShadowRelPos(int shadowDX, int shadowDY) { shadow.dx = shadowDX; shadow.dy = shadowDY; }
void Text::setShadowFadeFactor(float shadowFadeFactor)
{
    if(shadowFadeFactor<0.0f) shadowFadeFactor = 0.0f;
    if(shadowFadeFactor>1.0f) shadowFadeFactor = 1.0f;
    shadow.fadeFactor = shadowFadeFactor;
}
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
    if(txtTex!=nullptr) GLSDL_DestroyTexture(txtTex);
    txtTex = nullptr;
    runs.clear();
    anyStyledRun = false;
    lineCount = 0;
    linePitch = 0;
    tabAdvance = 0;
    width = 0;
    height = 0;

    if(!initted || rend==nullptr || font==nullptr) return;

    //An authored line height wins outright; otherwise lines sit exactly one glyph box apart. Note a
    //pitch below the font height is legal and deliberately lets consecutive lines overlap.
    linePitch = (lineHeight>0 ? lineHeight : TTF_FontHeight(font))+lineSpacing;
    tabAdvance = getEffTabAdvance();

    parseMarkup();

    std::vector<LineRange> lines;
    layoutLines(lines);
    lineCount = (int)lines.size();

    SDL_Surface* surf = buildSurface(lines);
    if(surf==nullptr) return;

    txtTex = GLSDL_CreateTextureFromSurface(rend, surf);
    SDL_FreeSurface(surf);
    if(txtTex==nullptr) {
        Log::warn(__PRETTY_FUNCTION__, "Texture creation failed");
        width = 0; height = 0; runs.clear();
        return;
    }

    int w, h;
    GLSDL_QueryTexture(txtTex, NULL, NULL, &w, &h);
    width = w;
    height = h;

    for(const Run& r : runs) {
        if(r.style.hasFg || r.style.hasBg) { anyStyledRun = true; break; }
    }
}

void Text::parseMarkup()
{
    plainText.clear();
    styleIds.clear();
    stylePalette.clear();
    stylePalette.push_back(Style());

    //Nothing to translate at all, so hand the string straight over.
    if(sentinel==0 && text.find(u'\\')==std::u16string::npos) { plainText = text; return; }

    plainText.reserve(text.size());
    styleIds.reserve(text.size());

    Style cur;
    uint16_t curId = 0;
    const size_t n = text.size();
    size_t i = 0;
    while(i<n) {
        char16_t c = text[i];

        //Two-character escapes, always honored. An unrecognized one is left exactly as written, so
        //only "\n", "\t" and "\\" ever change meaning.
        if(c==u'\\' && i+1<n) {
            char16_t esc = 0;
            switch(text[i+1]) {
                case u'n':  esc = u'\n'; break;
                case u't':  esc = u'\t'; break;
                case u'\\': esc = u'\\'; break;
            }
            if(esc!=0) {
                plainText.push_back(esc); styleIds.push_back(curId);
                i += 2;
                continue;
            }
        }

        if(sentinel!=0 && c==sentinel) {
            if(i+1<n && text[i+1]==sentinel) {
                plainText.push_back(sentinel); styleIds.push_back(curId);
                i += 2;
                continue;
            }
            if(i+1<n && text[i+1]==u'[') {
                size_t close = text.find(u']', i+2);
                if(close!=std::u16string::npos) {
                    std::u16string body = text.substr(i+2, close-(i+2));
                    if(body.empty()) cur = Style();
                    else applySpanAttribs(cur, body);
                    curId = internStyle(stylePalette, cur);
                    i = close+1;
                    continue;
                }
            }
            //A sentinel that opens nothing stays literal, so opting in never mangles ordinary text.
        }

        plainText.push_back(c); styleIds.push_back(curId);
        i++;
    }

    //Markup that resolved to nothing but the default style costs nothing to forget.
    if(stylePalette.size()<=1) styleIds.clear();
}

void Text::layoutLines(std::vector<LineRange>& lines) const
{
    lines.clear();
    const size_t n = plainText.size();
    const int maxW = getEffWrapLength();

    //Trailing blanks at a break would skew a centered or right-aligned line, so they never survive.
    //Leading ones are untouched, which is what keeps a tab-indented line indented.
    auto pushTrimmed = [&](size_t start, size_t len) {
        while(len>0 && isBreakSpace(plainText[start+len-1])) len--;
        LineRange lr; lr.start = start; lr.len = len;
        lines.push_back(lr);
    };

    size_t segStart = 0;
    while(true) {
        size_t nl = plainText.find(u'\n', segStart);
        size_t segEnd = (nl==std::u16string::npos ? n : nl);

        size_t cur = segStart;
        while(true) {
            size_t remaining = segEnd-cur;
            if(remaining==0) { pushTrimmed(cur, 0); break; }

            size_t fit = measureFitCount(font, plainText, cur, remaining, maxW, tabAdvance);
            if(fit>=remaining) { pushTrimmed(cur, remaining); break; }

            //Prefer the last blank at or before the overflow point; otherwise hard-split the word.
            size_t brk = std::u16string::npos;
            for(size_t j = cur+fit; j>cur; j--) {
                if(isBreakSpace(plainText[j-1])) { brk = j-1; break; }
            }

            if(brk!=std::u16string::npos && brk>cur) {
                pushTrimmed(cur, brk-cur);
                cur = brk+1;
                //Swallow the rest of the blank run, or the next line starts visibly indented.
                while(cur<segEnd && isBreakSpace(plainText[cur])) cur++;
                if(cur>=segEnd) break;
            } else {
                size_t cut = (fit>0 ? fit : 1);
                if(cur+cut<segEnd && isLowSurrogate(plainText[cur+cut])) cut = (cut>1 ? cut-1 : cut+1);
                if(cur+cut>segEnd) cut = segEnd-cur;
                pushTrimmed(cur, cut);
                cur += cut;
            }
        }

        if(segEnd>=n) break;
        segStart = segEnd+1;
    }

    if(maxLines>0 && (int)lines.size()>maxLines) {
        lines.resize(maxLines);
        LineRange& last = lines.back();
        int ellW = measureWidth(font, ELLIPSIS, 0, ELLIPSIS.size(), tabAdvance);
        while(last.len>0 && measureWidth(font, plainText, last.start, last.len, tabAdvance)+ellW>maxW) {
            last.len--;
            if(last.len>0 && isLowSurrogate(plainText[last.start+last.len])) last.len--;
        }
        last.ellipsized = true;
    }
}

void Text::buildLinePieces(const LineRange& line, std::vector<LinePiece>& out) const
{
    out.clear();

    const size_t end = line.start+line.len;
    size_t i = line.start;
    while(i<end) {
        uint16_t id = (i<styleIds.size() ? styleIds[i] : 0);

        if(plainText[i]==u'\t') {
            LinePiece p;
            p.styleId = id;
            p.isTab = true;
            p.w = tabAdvance;
            out.push_back(p);
            i++;
            continue;
        }

        size_t j = i;
        while(j<end && plainText[j]!=u'\t' && (j<styleIds.size() ? styleIds[j] : 0)==id) j++;

        LinePiece p;
        p.styleId = id;
        p.txt = plainText.substr(i, j-i);
        p.w = measureWidth(font, p.txt, 0, p.txt.size(), tabAdvance);
        out.push_back(p);
        i = j;
    }

    if(line.ellipsized) {
        LinePiece p;
        p.styleId = (line.len>0 && line.start+line.len-1<styleIds.size()) ? styleIds[line.start+line.len-1] : 0;
        p.txt = ELLIPSIS;
        p.w = measureWidth(font, ELLIPSIS, 0, ELLIPSIS.size(), tabAdvance);
        out.push_back(p);
    }
}

SDL_Surface* Text::buildSurface(const std::vector<LineRange>& lines)
{
    const int fontHeight = TTF_FontHeight(font);
    const int n = (int)lines.size();
    if(n<=0) return nullptr;

    std::vector<std::vector<LinePiece>> allPieces(n);
    std::vector<int> lineWidths(n, 0);
    int maxW = 0;
    for(int i = 0; i<n; i++) {
        buildLinePieces(lines[i], allPieces[i]);
        int lw = 0;
        for(const LinePiece& p : allPieces[i]) lw += p.w;
        lineWidths[i] = lw;
        if(lw>maxW) maxW = lw;
    }

    //A trailing line adds only its own glyph box, so getHeight() stays the text's real extent.
    const int totalH = (n-1)*linePitch+fontHeight;
    //Text with no drawable glyph at all reports 0x0, which keeps getHeight() usable as an
    //emptiness test. A blank line among non-blank ones still occupies its full pitch.
    if(maxW<=0 || totalH<=0) { width = 0; height = 0; return nullptr; }
    width = maxW;
    height = totalH;

    //One line of one style needs no compositing - its own surface already is the whole texture.
    if(n==1 && allPieces[0].size()==1 && !allPieces[0][0].isTab) {
        const LinePiece& only = allPieces[0][0];
        SDL_Surface* single = renderRunSurface(font, only.txt, 0, only.txt.size());
        if(single==nullptr) { width = 0; height = 0; return nullptr; }
        Run r;
        r.src = {0, 0, single->w, single->h};
        r.style = stylePalette[only.styleId<stylePalette.size() ? only.styleId : 0];
        runs.push_back(r);
        return single;
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, maxW, totalH, 32, SDL_PIXELFORMAT_ARGB8888);
    if(surf==nullptr) {
        Log::warn(__PRETTY_FUNCTION__, "Surface creation failed");
        width = 0; height = 0;
        return nullptr;
    }
    SDL_FillRect(surf, NULL, 0);

    for(int i = 0; i<n; i++) {
        int x = 0;
        switch(align) {
            case Align::CENTER: x = (maxW-lineWidths[i])/2; break;
            case Align::RIGHT:  x = maxW-lineWidths[i]; break;
            case Align::LEFT:   break;
        }
        const int lineTop = i*linePitch;

        for(const LinePiece& p : allPieces[i]) {
            if(p.isTab) {
                //Pure advance over transparent pixels, but it still belongs to whatever span it sits
                //in so a highlight reads as one unbroken bar across the gap.
                Run r;
                r.src = {x, lineTop, p.w, fontHeight};
                r.style = stylePalette[p.styleId<stylePalette.size() ? p.styleId : 0];
                runs.push_back(r);
                x += p.w;
                continue;
            }
            if(p.txt.empty() || p.w<=0) { x += p.w; continue; }

            SDL_Surface* ps = renderRunSurface(font, p.txt, 0, p.txt.size());
            if(ps!=nullptr) {
                //Runs never overlap, so copying rather than blending keeps their alpha intact.
                SDL_SetSurfaceBlendMode(ps, SDL_BLENDMODE_NONE);
                SDL_Rect dst = {x, lineTop, ps->w, ps->h};
                SDL_BlitSurface(ps, NULL, surf, &dst);

                Run r;
                r.src = {x, lineTop, ps->w, ps->h};
                r.style = stylePalette[p.styleId<stylePalette.size() ? p.styleId : 0];
                runs.push_back(r);

                SDL_FreeSurface(ps);
            }
            x += p.w;
        }
    }

    return surf;
}

void Text::drawRunPass(int bx, int by, bool shadowPass) const
{
    for(const Run& r : runs) {
        Color fg = (r.style.hasFg ? r.style.fg : textColor);

        SDL_Rect src = r.src;
        SDL_Rect dst;
        dst.x = bx+(int)(r.src.x*scale);
        dst.y = by+(int)(r.src.y*scale);
        dst.w = (int)(r.src.w*scale);
        dst.h = (int)(r.src.h*scale);

        if(shadowPass) {
            dst.x += (int)(shadow.dx*scale);
            dst.y += (int)(shadow.dy*scale);
            Color sc = getShadowColorFor(fg);
            GLSDL_SetTextureColorMod(txtTex, sc.r, sc.g, sc.b);
            GLSDL_SetTextureAlphaMod(txtTex, 255*shadow.fadeFactor);
        } else {
            GLSDL_SetTextureColorMod(txtTex, fg.r, fg.g, fg.b);
            GLSDL_SetTextureAlphaMod(txtTex, fg.a);
        }

        GLSDL_RenderCopy(rend, txtTex, &src, &dst);
    }
}

Color Text::getShadowColorFor(const Color& fg) const
{
    if(shadow.customColor.a==0) return Color(255-fg.r, 255-fg.g, 255-fg.b, 255);
    return shadow.customColor;
}

int Text::getEffWrapLength() const
{
    //A non-positive wrap length means "break on explicit newlines only", matching SDL_ttf.
    return wrapLength>0 ? wrapLength : INT_MAX;
}

int Text::getEffTabAdvance() const
{
    if(tabWidth>0) return tabWidth;

    //Four spaces is the conventional stop, and deriving it tracks the font instead of a magic number.
    int spaceW = measureWidth(font, u" ", 0, 1, 0);
    if(spaceW<=0) spaceW = TTF_FontHeight(font)/4;
    return spaceW*4>0 ? spaceW*4 : 1;
}

bool Text::Style::matches(const Style& o) const
{
    return hasFg==o.hasFg && hasBg==o.hasBg && fg==o.fg && bg==o.bg;
}

bool Text::isLowSurrogate(char16_t c) { return c>=0xDC00 && c<=0xDFFF; }

bool Text::isBreakSpace(char16_t c) { return c==u' ' || c==u'\t'; }

int Text::measureWidth(TTF_Font* font, const std::u16string& txt, size_t start, size_t len, int tabAdvance)
{
    if(len==0 || start>=txt.size()) return 0;
    const size_t end = (start+len<txt.size()) ? start+len : txt.size();

    //A tab has no glyph to measure, so the string is measured in the runs between tabs.
    int total = 0;
    size_t i = start;
    while(i<end) {
        if(txt[i]==u'\t') { total += tabAdvance; i++; continue; }

        size_t j = i;
        while(j<end && txt[j]!=u'\t') j++;
        std::u16string sub = txt.substr(i, j-i);
        int w = 0, h = 0;
        TTF_SizeUNICODE(font, reinterpret_cast<const Uint16*>(sub.c_str()), &w, &h);
        total += w;
        i = j;
    }
    return total;
}

size_t Text::measureFitCount(TTF_Font* font, const std::u16string& txt, size_t start, size_t len, int maxWidth, int tabAdvance)
{
    if(len==0 || start>=txt.size()) return 0;
    const size_t end = (start+len<txt.size()) ? start+len : txt.size();

    int budget = maxWidth;
    size_t i = start;
    while(i<end) {
        if(txt[i]==u'\t') {
            if(tabAdvance>budget) return i-start;
            budget -= tabAdvance;
            i++;
            continue;
        }

        size_t j = i;
        while(j<end && txt[j]!=u'\t') j++;
        std::u16string sub = txt.substr(i, j-i);

        int extent = 0, count = 0;
        if(TTF_MeasureUNICODE(font, reinterpret_cast<const Uint16*>(sub.c_str()), budget, &extent, &count)!=0) {
            return len;
        }
        if(count<0) count = 0;
        size_t fit = (size_t)count;
        if(fit>sub.size()) fit = sub.size();

        if(fit<sub.size()) {
            //Never report a boundary that lands between the halves of a surrogate pair.
            if(isLowSurrogate(sub[fit])) fit = (fit>0 ? fit-1 : 0);
            return (i-start)+fit;
        }

        budget -= extent;
        i = j;
    }
    return end-start;
}

SDL_Surface* Text::renderRunSurface(TTF_Font* font, const std::u16string& txt, size_t start, size_t len)
{
    if(len==0) return nullptr;
    std::u16string sub = txt.substr(start, len);
    //Always baked white: every color, span colors included, is a draw-time modulation.
    return TTF_RenderUNICODE_Blended(font, reinterpret_cast<const Uint16*>(sub.c_str()), {255, 255, 255, 255});
}

void Text::applySpanAttribs(Style& style, const std::u16string& body)
{
    size_t i = 0;
    while(i<body.size()) {
        while(i<body.size() && body[i]==u' ') i++;
        size_t j = i;
        while(j<body.size() && body[j]!=u' ') j++;
        if(j<=i) { i = j; continue; }

        //Attribute names and hex values are ASCII by construction; anything else cannot be either.
        std::string tok;
        tok.reserve(j-i);
        for(size_t k = i; k<j; k++) tok.push_back(body[k]<128 ? (char)body[k] : '?');
        i = j;

        std::string key, val;
        size_t eq = tok.find('=');
        if(eq!=std::string::npos) {
            key = tok.substr(0, eq);
            val = tok.substr(eq+1);
        } else if(tok[0]=='#') {
            key = "fg";
            val = tok.substr(1);
        } else {
            Log::warnv(__PRETTY_FUNCTION__, "ignoring it", "Unrecognized span attribute \"%s\"", tok.c_str());
            continue;
        }

        for(size_t k = 0; k<key.size(); k++) key[k] = (char)std::tolower((unsigned char)key[k]);
        bool isFg = (key=="fg" || key=="color");
        bool isBg = (key=="bg" || key=="hl" || key=="highlight");
        if(!isFg && !isBg) {
            Log::warnv(__PRETTY_FUNCTION__, "ignoring it", "Unrecognized span attribute \"%s\"", tok.c_str());
            continue;
        }

        if(val=="none" || val=="-") {
            if(isFg) style.hasFg = false;
            else     style.hasBg = false;
            continue;
        }

        try {
            Color c = Color::fromStringB16(val);
            if(isFg) { style.fg = c; style.hasFg = true; }
            else     { style.bg = c; style.hasBg = true; }
        } catch(...) {
            Log::warnv(__PRETTY_FUNCTION__, "ignoring it", "Unparseable span color \"%s\"", val.c_str());
        }
    }
}

uint16_t Text::internStyle(std::vector<Style>& palette, const Style& style)
{
    for(size_t i = 0; i<palette.size(); i++) {
        if(palette[i].matches(style)) return (uint16_t)i;
    }
    if(palette.size()>=0xFFFF) return 0;
    palette.push_back(style);
    return (uint16_t)(palette.size()-1);
}
