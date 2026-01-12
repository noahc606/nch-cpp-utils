#include "Poly.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <nch/cpp-utils/arraylist.h>
#include "Tri.h"
#include "Quad.h"
#include "GeoUtils.h"

Poly::Poly(){}
Poly::Poly(const std::vector<Vertex>& verts)
{
    Poly::verts = verts;
    super_updateNormals();
}
Poly::~Poly(){}

int Poly::getType() const {
    return type;
}
int Poly::getNumVerts() const {
    return verts.size();
}
int Poly::vs() const {
    return getNumVerts();
}
Vertex Poly::v(int idx) const {
    return verts[idx];
}
glm::vec3 Poly::norm() const {
    return normal;
}

void Poly::simplyTex(const glm::vec2& uv0, const glm::vec2& uv1)
{
    glm::vec3 col0 = {1.0, 1.0, 1.0};
    glm::vec3 col1 = {0.85, 0.85, 0.85};
    glm::vec3 col2 = {0.7, 0.7, 0.7};

    if(verts.size()==3) {
        (*this) = Tri(
            {v(0).pos, glm::vec2(uv0.x, uv1.y)},
            {v(1).pos, glm::vec2(uv1.x, uv1.y)},
            {v(2).pos, glm::vec2(uv0.x, uv0.y)}
        );
        return;
    }
    if(verts.size()==4) {
        //Up-facing or Down-facing quad...
        if(norm().y==1) {
            (*this) = Quad(
                {v(0).pos, {uv0.x, uv0.y}},
                {v(1).pos, {uv0.x, uv1.y}},
                {v(2).pos, {uv1.x, uv1.y}},
                {v(3).pos, {uv1.x, uv0.y}}
            ); return;
        }
        if(norm().y==-1) {
            (*this) = Quad(
                {v(0).pos, {uv1.x, uv1.y}},
                {v(1).pos, {uv1.x, uv0.y}},
                {v(2).pos, {uv0.x, uv0.y}},
                {v(3).pos, {uv0.x, uv1.y}}
            ); return;
        }

        //Side-facing quad
        if(norm().y==0) {
            //X
            if(norm().x!=0) {
                //-X
                if(norm().x<0) {
                    (*this) = Quad(
                        {v(0).pos, col1, {uv0.x, uv1.y}},
                        {v(1).pos, col1, {uv1.x, uv1.y}},
                        {v(2).pos, col1, {uv1.x, uv0.y}},
                        {v(3).pos, col1, {uv0.x, uv0.y}}
                    ); return;
                //+X
                } else {
                    (*this) = Quad(
                        {v(0).pos, col1, {uv1.x, uv1.y}},
                        {v(1).pos, col1, {uv1.x, uv0.y}},
                        {v(2).pos, col1, {uv0.x, uv0.y}},
                        {v(3).pos, col1, {uv0.x, uv1.y}}
                    ); return;
                }

            //Z
            } else {
                (*this) = Quad(
                    {v(0).pos, col2, {uv0.x, uv0.y}},
                    {v(1).pos, col2, {uv0.x, uv1.y}},
                    {v(2).pos, col2, {uv1.x, uv1.y}},
                    {v(3).pos, col2, {uv1.x, uv0.y}}
                ); return;
            }
        }

        //Down-slanted quad...
        if(norm().y<0) {
            (*this) = Quad(
                {v(0).pos, {uv0.x, uv0.y}},
                {v(1).pos, {uv0.x, uv1.y}},
                {v(2).pos, {uv1.x, uv1.y}},
                {v(3).pos, {uv1.x, uv0.y}}
            ); return;
        } else {
        //Up-slanted quad...

        }
        
        //Other... (unimplemented)
        (*this) = Quad(
            {v(0).pos, {uv0.x, uv0.y}},
            {v(1).pos, {uv0.x, uv1.y}},
            {v(2).pos, {uv1.x, uv1.y}},
            {v(3).pos, {uv1.x, uv0.y}}
        ); return;
    }
}
void Poly::rotate(const glm::vec3& center, const glm::vec3& xyzRot)
{
    for(int i = 0; i<verts.size(); i++) {
        GeoUtils::rotatePoint(verts[i].pos, center, xyzRot);
    }
    super_updateNormals();
}
void Poly::move(const glm::vec3& offset)
{
    for(int i = 0; i<verts.size(); i++) {
        verts[i].pos += offset;
    }
}

void Poly::super_type(int type) {
    if(Poly::type==PolyType::UNKNOWN) {
        Poly::type = type;
    } else {
        assert(false);
    }
}
void Poly::super_updateNormals() {

    if(verts.size()==3) {
        normal = glm::cross((v(1).pos-v(0).pos), (v(2).pos-v(0).pos));
        verts[0].normal = normal;
        verts[1].normal = normal;
        verts[2].normal = normal;
    }
    if(verts.size()==4) {
        normal = Tri(v(0), v(1), v(2)).norm();
        verts[0].normal = normal;
        verts[1].normal = normal;
        verts[2].normal = normal;
        verts[3].normal = normal;
    }
}
void Poly::super_updateColors() {
    glm::vec3 polyCol = {1.0, 0.0, 0.0};

}