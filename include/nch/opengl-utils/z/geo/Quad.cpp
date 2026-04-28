#include "Quad.h"
#include "Tri.h"
#include <nch/cpp-utils/log.h>
#include <nch/math-utils/box2.h>
using namespace nch;

Quad::Quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, float expansion)
{
    super_type(Poly::QUAD);
    verts = { v0, v1, v2, v3 };
    super_updateNormals();
    super_updateColors();
    if(expansion != 0.0f) expand(expansion);
}