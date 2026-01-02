#include "Text.h"
#include "TexUtils.h"
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <codecvt>
#include <cstddef>
#include <locale>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/string-utils.h>
#include <string>
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

void Text::drawCentered(int x, int y, int w, int h)
{
    draw(x+w/2-(int)getWidth()/2, y+h/2-(int)getHeight()/2);
}

void Text::stream(SDL_Renderer* rend, TTF_Font* font, std::string text, const Color& c, int x, int y, double scale)
{
    int textWidth = 0;
    TTF_MeasureText(font, text.c_str(), 5000, &textWidth, NULL);
    int textHeight = TTF_FontHeight(font);

    SDL_Surface* txtSurf = TTF_RenderText_Blended(font, text.c_str(), {255, 255, 255});
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

bool Text::isInitialized() { return initted; }
double Text::getScale() { return scale; }
double Text::getWidth() { return width*scale; }
double Text::getUnscaledWidth() { return width; }
double Text::getHeight() { return height*scale; }
double Text::getUnscaledHeight() { return height; }
std::u16string Text::getText() { return text; }

bool Text::setScale(double scale)
{
    if(scale==Text::scale) return false;

    Text::scale = scale;
    updateTextTexture();
    return true;
}
void Text::forcedNearestScaling(bool fns)
{
    forceNearestScaling = fns;
}

bool Text::setText(std::u16string text)
{
    if(text==Text::text) return false;
    
    //Update string and update unscaled width/height
    Text::text = text;
    updateTextTexture();
    return true;
}
bool Text::setText(std::string text)
{
    //Convert string to unicode and set text
    std::string utf8_string = text;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string utf16_string = convert.from_bytes(utf8_string);
    const char16_t* unicodeText = utf16_string.c_str();

    return setText(unicodeText);
}

void Text::setWrapLength(int wl)
{
    if(wl==wrapLength) return;
    wrapLength = wl;

    if(text==u"") return;
    updateTextTexture();
}
void Text::setMaxLines(int ml)
{
    if(ml<1) ml = -1;
    if(ml==maxLines) return;
    maxLines = ml;

    if(text==u"") return;
    updateTextTexture();
}
void Text::setEveryLineCentered(bool elc)
{
    if(elc==everyLineCentered) return;
    everyLineCentered = elc;

    if(text==u"") return;
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
    //Destroy the last texture if it exists and re-create it from the 'txtSurf'
    if(txtTex!=nullptr) SDL_DestroyTexture(txtTex);
    txtTex = nullptr;

    /* Use text width processing on some occassions */
    if(maxLines>0 || everyLineCentered) {
        int fontHeight = TTF_FontHeight(font);
        auto processed = getProcessedText(text, font, wrapLength, maxLines);
        txtTex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, wrapLength, processed.size()*fontHeight);
        TexUtils::clearTexture(rend, txtTex);
        
        for(int i = 0; i<processed.size(); i++) {
            auto elem = processed[i];
            SDL_Surface* lineSurf = TTF_RenderUNICODE_Blended(font, reinterpret_cast<const Uint16*>(elem.second.c_str()), {255, 255, 255, 255});
            if(lineSurf==NULL) continue;
            SDL_Texture* lineTex = SDL_CreateTextureFromSurface(rend, lineSurf);
            if(lineTex==NULL) continue;

            SDL_SetRenderTarget(rend, txtTex); {
                SDL_Rect dst;
                dst.y = (fontHeight)*i+1;
                dst.h = lineSurf->h;
                dst.w = lineSurf->w;
                if(everyLineCentered) {
                    dst.x = (wrapLength/2-elem.first/2)+1;
                } else {
                    dst.x = 0;
                }
                SDL_RenderCopy(rend, lineTex, NULL, &dst);
            } SDL_SetRenderTarget(rend, NULL);

            SDL_DestroyTexture(lineTex);
            SDL_FreeSurface(lineSurf);
        }
    } else {
        //Create surface representing the current text
        SDL_Surface* txtSurf = TTF_RenderUNICODE_Blended_Wrapped(font, reinterpret_cast<const Uint16*>(text.c_str()), {255, 255, 255, 255}, wrapLength);
        if(txtSurf==NULL) return;
        txtTex = SDL_CreateTextureFromSurface(rend, txtSurf);
        SDL_FreeSurface(txtSurf);
    }

    
    
    //Set width and height, destroy the txtSurf.
    int w, h;
    SDL_QueryTexture(txtTex, NULL, NULL, &w, &h);
    width = w;
    height = h;
}

int Text::measureTextWidth(TTF_Font* font, const std::u16string& text) {
    int w = 0, h = 0;
    TTF_SizeUNICODE(font, reinterpret_cast<const Uint16*>(text.c_str()), &w, &h);
    return w;
}

std::vector<std::pair<int, std::u16string>> Text::getProcessedText(const std::u16string& text, TTF_Font* font, int maxWidth, int maxLines) {
    auto isHighSurrogate = [](char16_t c){ return c>=0xD800 && c<=0xDBFF; };
    auto isLowSurrogate  = [](char16_t c){ return c>=0xDC00 && c<=0xDFFF; };

    std::vector<std::pair<int, std::u16string>> lines;
    std::u16string currentLine;
    std::u16string currentWord;

    auto pushLine = [&](const std::u16string& l){
        lines.emplace_back(measureTextWidth(font, l), l);
    };

    //Append one word into lines/currentLine. Splits a very long word into fragments.
    auto appendWordToBuffer = [&](std::u16string word) {
        //First, try to append to existing currentLine with a space
        if(!currentLine.empty()) {
            std::u16string test = currentLine + u' '+word;
            if(measureTextWidth(font, test)<=maxWidth) {
                currentLine = std::move(test);
                return;
            }
            //else push currentLine and continue with empty currentLine
            pushLine(currentLine);
            currentLine.clear();
        }

        //Now currentLine is empty. Split 'word' into fragments that fit maxWidth.
        while(!word.empty()) {
            // Binary search for the longest prefix that fits
            int lo = 1;
            int hi = static_cast<int>(word.size());
            int best = 0;
            while(lo<=hi) {
                int mid = (lo + hi) / 2;

                //Avoid splitting a surrogate pair: if prefix ends on a high surrogate, back it up.
                if (mid>0 && isHighSurrogate(word[mid-1])) mid--;
                if (mid<=0) { hi = mid-1; continue; }

                std::u16string prefix = word.substr(0, mid);
                if (measureTextWidth(font, prefix) <= maxWidth) {
                    best = mid;
                    lo = mid+1;
                } else {
                    hi = mid-1;
                }
            }

            if(best == 0) {
                //No prefix fits: take the smallest valid unit (1 or 2 code units if surrogate pair)
                int take = 1;
                if(isHighSurrogate(word[0]) && word.size()>1) take = 2;
                std::u16string part = word.substr(0, take);
                pushLine(part);
                word.erase(0, take);
                continue;
            }

            if(best >= static_cast<int>(word.size())) {
                currentLine = std::move(word); //whole remainder fits so keep it in currentLine
                break;
            } else {
                //push this fragment as a full line and continue
                std::u16string part = word.substr(0, best);
                pushLine(part);
                word.erase(0, best);
            }
        }
    };

    //Parse input text into words (split on spaces/newlines). Respect explicit newlines.
    for(char16_t c : text) {
        if(c==u' ' || c==u'\n') {
            if(!currentWord.empty()) {
                appendWordToBuffer(std::move(currentWord));
                currentWord.clear();
            }
            if(c==u'\n') {
                if (!currentLine.empty()) {
                    pushLine(currentLine);
                    currentLine.clear();
                }
            }
        } else {
            currentWord.push_back(c);
        }
    }
    if(!currentWord.empty()) appendWordToBuffer(std::move(currentWord));
    if(!currentLine.empty()) pushLine(currentLine);

    //If too many lines, truncate and append ellipsis "..."
    if(static_cast<int>(lines.size())>maxLines) {
        lines.resize(maxLines);
        const std::u16string ellipsis = u"...";
        std::u16string& last = lines.back().second;
        int ellW = measureTextWidth(font, ellipsis);
        int w = measureTextWidth(font, last);
        while (!last.empty() && w+ellW>maxWidth) {
            last.pop_back();
            w = measureTextWidth(font, last);
        }
        last += ellipsis;
        lines.back().first = measureTextWidth(font, last);
    }

    return lines;
}