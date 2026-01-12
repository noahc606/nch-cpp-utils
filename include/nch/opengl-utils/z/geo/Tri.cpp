#include "Tri.h"
#include <nch/cpp-utils/log.h>
using namespace nch;

Tri::Tri(const Vertex& v0, const Vertex& v1, const Vertex& v2)
{
    super_type(Poly::TRI);  //Set type
    verts = { v0, v1, v2 }; //Set vertices
    super_updateNormals();  //Set normals of vertices and of the poly
    super_updateColors();
}