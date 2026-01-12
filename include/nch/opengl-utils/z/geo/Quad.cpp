#include "Quad.h"
#include "Tri.h"
#include <nch/cpp-utils/log.h>
#include <nch/math-utils/box2.h>
using namespace nch;

Quad::Quad(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    super_type(Poly::QUAD);     //Set type
    verts = { v0, v1, v2, v3 }; //Set vertices
    super_updateNormals();      //Set normals of vertices and of the poly
    super_updateColors();
}