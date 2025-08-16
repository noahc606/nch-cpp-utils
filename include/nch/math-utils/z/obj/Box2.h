#pragma once
#include <algorithm>
#include "Vec2.h"

namespace nch { template <typename T = int> class Box2
{
public:
    Box2(){}
    Box2(T x1, T y1, T x2, T y2) {
        c1.x = std::min(x1, x2);    c2.x = std::max(x1, x2);
        c1.y = std::min(y1, y2);    c2.y = std::max(y1, y2);
    }
    Box2(Vec2<T> c1, Vec2<T> c2): Box2(c1.x, c1.y, c2.x, c2.y){}

    

    /* Getters */
    T getWidth() const {
        return c2.x-c1.x;
    }
    T getHeight() const {
        return c2.y-c1.y;
    }
    bool contains(Vec2<T> p) {
        return (c1.x<=p.x && p.x<=c2.x && c1.y<=p.y && p.y<=c2.y);
    }
    bool contains(Box2<T> b) {
        return (c1.x<=b.c1.x && c1.y<=b.c1.y && c2.x>=b.c2.x && c2.y>=b.c2.y);
    }
    bool intersects(Box2<T> b) {
        return (c1.x<=b.c2.x && b.c1.x<=c2.x && c1.y<=b.c2.y && b.c1.y<=c2.y);
    }
    bool collides(Box2<T> b) {
        return (c1.x<b.c2.x && b.c1.x<c2.x && c1.y<b.c2.y && b.c1.y<c2.y);
    }
    bool operator==(Box2<T> other) {
        return (c1.x==other.c1.x && c1.y==other.c1.y && c2.x==other.c2.x && c2.y==other.c2.y);
    }
    static Box2<T> createFromXYWH(T x, T y, T w, T h) {
        return Box2<T>(x, y, x+w, y+h);
    }

    /* Mutators */
    void scale(T scaleFactor) {
        c1 *= scaleFactor;
        c2 *= scaleFactor;
    }

    Vec2<T> c1; //Corner 1
    Vec2<T> c2; //Corner 2
protected:

private:

};

typedef Box2<int> Box2i;
typedef Box2<int64_t> Box2i64;
typedef Box2<uint64_t> Box2u64;
typedef Box2<float> Box2f;
typedef Box2<double> Box2d;
}
