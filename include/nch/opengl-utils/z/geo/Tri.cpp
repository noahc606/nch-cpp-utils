#include "Tri.h"
#include <nch/cpp-utils/log.h>
using namespace nch;

Tri::Tri(const Vertex& v0, const Vertex& v1, const Vertex& v2, float expansion)
{
    super_type(Poly::TRI);
    verts = { v0, v1, v2 };
    super_updateNormals();
    super_updateColors();
    if(expansion != 0.0f) expand(expansion);
}