#pragma once
#include "Poly.h"
#include "Vertex.h"

class Tri : public Poly {
public:
    Tri(const Vertex& v0, const Vertex& v1, const Vertex& v2);

    Tri moved(glm::i64vec3 offset);
protected:
private:
};