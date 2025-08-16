#pragma once
#include <SDL2/SDL.h>

namespace nch { class FRect {
public:
    FRect(){}
    FRect(float x, float y, float w, float h) {
        r.x = x; 
        r.y = y;
        r.w = w;
        r.h = h;
    }
    FRect(const SDL_FRect& r) { FRect::r = r; }
    ~FRect(){}

    /* Getters */
    int x1() const { return r.x; }
    int y1() const { return r.y; }
    int x2() const { return r.x+r.w; }
    int y2() const { return r.y+r.h; }
    static FRect createFromTwoPts(float x1, float y1, float x2, float y2) { return FRect(x1, y1, x2-x1, y2-y1); }
    bool intersects(const FRect& b) const {
        return (
            r.x<=b.r.x+b.r.w && b.r.x<=r.x+r.w &&
            r.y<=b.r.y+b.r.h && b.r.y<=r.y+r.h
        );
    }
    bool contains(float x, float y) const {
        return (
            r.x<=x && x<=r.x+r.w &&
            r.y<=y && y<=r.y+r.h
        );
    };
    
    bool operator==(const FRect& other) const {
        return (
            r.x==other.r.x &&
            r.y==other.r.y &&
            r.w==other.r.w &&
            r.h==other.r.h
        );
    }
    bool operator!=(const FRect& other) const {
        return !(*this==other);
    }
    
    /* Mutators */
    void scale(float factor) {
        r.x *= factor;
        r.y *= factor;
        r.w *= factor;
        r.h *= factor;
    }
    void translate(float dx, float dy) {
        r.x += dx;
        r.y += dy;
    }

    SDL_FRect r;
private:
}; }