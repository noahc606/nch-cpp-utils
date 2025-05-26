#pragma once
#include <SDL2/SDL.h>

namespace nch { class Rect {
public:
    Rect(){}
    Rect(int x, int y, int w, int h) {
        r.x = x; 
        r.y = y;
        r.w = w;
        r.h = h;
    }
    Rect(const SDL_Rect& r) { Rect::r = r; }
    ~Rect(){}

    /* Getters */
    bool isEmpty() { return r.w==0 || r.h==0; }
    int x1() const { return r.x; }
    int y1() const { return r.y; }
    int x2() const { return r.x+r.w; }
    int y2() const { return r.y+r.h; }
    int midX() const { return r.x+r.w/2; }
    int midY() const { return r.y+r.h/2; }
    static Rect createFromTwoPts(int x1, int y1, int x2, int y2) { return Rect(x1, y1, x2-x1, y2-y1); }
    bool intersects(const Rect& b) {
        return (
            r.x<=b.r.x+b.r.w && b.r.x<=r.x+r.w &&
            r.y<=b.r.y+b.r.h && b.r.y<=r.y+r.h
        );
    }
    bool contains(int x, int y) {
        return (
            r.x<=x && x<=r.x+r.w &&
            r.y<=y && y<=r.y+r.h
        );
    };
    
    bool operator==(const Rect& other) {
        return (
            r.x==other.r.x &&
            r.y==other.r.y &&
            r.w==other.r.w &&
            r.h==other.r.h
        );
    }
    bool operator!=(const Rect& other) {
        return !(*this==other);
    }
    
    /* Mutators */
    void scale(float factor) {
        r.x *= factor;
        r.y *= factor;
        r.w *= factor;
        r.h *= factor;
    }
    void translate(int dx, int dy) {
        r.x += dx;
        r.y += dy;
    }

    SDL_Rect r;
private:
}; }