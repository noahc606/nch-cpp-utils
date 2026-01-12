#pragma once
#include <SDL2/SDL.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <vector>
#include <nch/math-utils/vec2.h>
#include <nch/sdl-utils/rect.h>

class MaxRectsBin {
public:
    MaxRectsBin(int width, int height);

    bool insert(const nch::Vec2i& imgDims, nch::Rect& outRect);

private:
    int binWidth, binHeight;
    std::vector<nch::Rect> freeRects;

    void placeRect(const nch::Rect& placed);
    std::vector<nch::Rect> mergeFreeList(std::vector<nch::Rect>& list);
    bool isContainedIn(const nch::Rect& a, const nch::Rect& b) const;

    bool rectsOverlap(const nch::Rect& a, const nch::Rect& b) const {
        return !(a.r.x >= b.r.x + b.r.w || 
                 a.r.x + a.r.w <= b.r.x || 
                 a.r.y >= b.r.y + b.r.h || 
                 a.r.y + a.r.h <= b.r.y);
    }
};