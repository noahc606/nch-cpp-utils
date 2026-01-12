#include "MaxRectsBin.h"
using namespace nch;

MaxRectsBin::MaxRectsBin(int width, int height)
    : binWidth(width), binHeight(height) {
    freeRects.push_back({0, 0, width, height});
}

bool MaxRectsBin::insert(const Vec2i& imgDims, Rect& outRect) {
    int bestShortSideFit = INT_MAX;
    int bestLongSideFit  = INT_MAX;
    size_t bestIndex = SIZE_MAX;
    Rect bestNode = {};

    // Find best fitting free rect
    for (size_t i = 0; i < freeRects.size(); ++i) {
        const Rect& r = freeRects[i];
        if (imgDims.x<=r.r.w && imgDims.y <= r.r.h) {
            int leftoverH = std::abs(r.r.h - imgDims.y);
            int leftoverW = std::abs(r.r.w - imgDims.x);
            int shortSideFit = std::min(leftoverW, leftoverH);
            int longSideFit  = std::max(leftoverW, leftoverH);

            if (shortSideFit < bestShortSideFit ||
               (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit)) {
                bestNode = { r.r.x, r.r.y, imgDims.x, imgDims.y };
                bestShortSideFit = shortSideFit;
                bestLongSideFit  = longSideFit;
                bestIndex = i;
            }
        }
    }

    if (bestIndex == SIZE_MAX)
        return false; // no fit found

    placeRect(bestNode);
    outRect = bestNode;
    return true;
}

void MaxRectsBin::placeRect(const Rect& placed) {
    std::vector<Rect> newFreeRects;
    for (auto& free : freeRects) {
        if (!rectsOverlap(free, placed)) {
            newFreeRects.push_back(free);
            continue;
        }

        // Split the free rect into up to 4 rectangles
        if (placed.r.x > free.r.x && placed.r.x < free.r.x + free.r.w) {
            newFreeRects.push_back({ free.r.x, free.r.y, placed.r.x - free.r.x, free.r.h });
        }
        if (placed.r.x + placed.r.w < free.r.x + free.r.w) {
            newFreeRects.push_back({ placed.r.x + placed.r.w, free.r.y,
                                     (free.r.x + free.r.w) - (placed.r.x + placed.r.w), free.r.h });
        }
        if (placed.r.y > free.r.y && placed.r.y < free.r.y + free.r.h) {
            newFreeRects.push_back({ free.r.x, free.r.y, free.r.w, placed.r.y - free.r.y });
        }
        if (placed.r.y + placed.r.h < free.r.y + free.r.h) {
            newFreeRects.push_back({ free.r.x, placed.r.y + placed.r.h,
                                     free.r.w, (free.r.y + free.r.h) - (placed.r.y + placed.r.h) });
        }
    }

    freeRects = mergeFreeList(newFreeRects);
}

std::vector<Rect> MaxRectsBin::mergeFreeList(std::vector<Rect>& list) {
    // Remove contained rectangles
    for (size_t i = 0; i < list.size(); ++i) {
        for (size_t j = i + 1; j < list.size(); ) {
            if (isContainedIn(list[i], list[j])) {
                list.erase(list.begin() + i);
                --i;
                break;
            } else if (isContainedIn(list[j], list[i])) {
                list.erase(list.begin() + j);
            } else {
                ++j;
            }
        }
    }
    return list;
}

bool MaxRectsBin::isContainedIn(const Rect& a, const Rect& b) const {
    return a.r.x >= b.r.x && a.r.y >= b.r.y &&
           a.r.x + a.r.w <= b.r.x + b.r.w &&
           a.r.y + a.r.h <= b.r.y + b.r.h;
}
