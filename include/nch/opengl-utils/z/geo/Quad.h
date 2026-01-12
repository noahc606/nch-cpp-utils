#pragma once
#include "Poly.h"
#include "Vertex.h"

/*
    Quad whose points are all on a single plane
*/
class Quad : public Poly {
public:
    Quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3);
private:
};