#include "Mesh.h"
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include <set>
#include <sstream>
#include "nch/opengl-utils/geo.h"
#include "nch/opengl-utils/xbo.h"

using namespace nch;

Mesh::Mesh(const std::vector<Atlas*>& atlases)
{
    Mesh::atlases = atlases;
}
Mesh::~Mesh() {
    reset();
}

void Mesh::draw(Shader* shader, Camera3D* cam)
{
    assert(built);
    if(!built) return;
    shader->useProgram();

    unsigned int numDiffuse = 0;
    unsigned int numSpecular = 0;
    for(unsigned int i = 0; i<atlases.size(); i++) {
        std::string num;
        std::string type = atlases[i]->getType();
        if(type=="diffuse") {
            num = std::to_string(numDiffuse++);
        } else if(type=="specular") {
            num = std::to_string(numSpecular++);
        }

        atlases[i]->texUnit(shader, (type+num).c_str(), i);
        atlases[i]->bind();
    }

    VAO::bind(glVAO);
    assert(glIsVertexArray(glVAO) == GL_TRUE);
    glDrawElements(GL_TRIANGLES, drawnIndexCount, GL_UNSIGNED_INT, nullptr);
    VAO::unbind();
}

int Mesh::getInternalVerticesSize() {
    return vertices.size();
}
int Mesh::getInternalIndicesSize() {
    return indices.size();
}
bool Mesh::isBuilt() {
    return built;
}

void Mesh::applyUpdates()
{
    if(built) {
        cleanup();
    }
    setup();
}
void Mesh::reset() {
    if(built) {
        cleanup();
    }
    polyMap.clear();
    indicesMap.clear();

    vertices.clear();
    indices.clear();
    drawnIndexCount = 0;
}
void Mesh::addPoly(const glm::ivec3& key, const Poly& poly)
{
    //Add the poly to the 'polyMap'
    polyMap.insert({{key.x, key.y, key.z}, poly});

    //Add vertices and indices to 'vertices', 'indices', and 'indicesMap'
    GLuint vs = vertices.size();
    if(poly.getNumVerts()==3) {
        for(int i = 0; i<3; i++) vertices.push_back(poly.v(i));
        indices.insert   (indices.end(),          {vs+0, vs+1, vs+2});
        indicesMap.insert({{key.x, key.y, key.z}, {vs+0, vs+1, vs+2}});
    }
    else if (poly.getNumVerts()==4) {
        for(int i = 0; i<4; i++) vertices.push_back(poly.v(i));
        indices.insert   (indices.end(),          {vs+0, vs+1, vs+2, vs+0, vs+2, vs+3});
        indicesMap.insert({{key.x, key.y, key.z}, {vs+0, vs+1, vs+2, vs+0, vs+2, vs+3}});
    }
}
void Mesh::addPoly(const Poly& poly) {
    addPoly({-1,-1,-1}, poly);
}

void Mesh::remove(const glm::ivec3& key)
{
    std::multimap<std::tuple<int, int, int>, Mesh::MPoly>::iterator pit;
    while((pit = polyMap.find({key.x, key.y, key.z}))!=polyMap.end()) {
        polyMap.erase(pit);
    }

    std::multimap<std::tuple<int, int, int>, std::vector<GLuint>>::iterator iit;
    while((iit = indicesMap.find({key.x, key.y, key.z}))!=indicesMap.end()) {
        std::set<GLuint> verticesToRemove;
        for(GLuint& vidx : iit->second) {
            verticesToRemove.insert(vidx);
        }
        GLuint beg = *verticesToRemove.begin();
        GLuint end = *verticesToRemove.rbegin();
        GLuint numVertsRemoved = end-beg+1;
        assert(numVertsRemoved==3 || numVertsRemoved==4);

        //Remove appropriate vertex indices.
        int numIndicesRemoved = 0;
        for(int i = indices.size()-1; i>=0; i--) {
            if(indices[i]>=beg && indices[i]<=end) {
                indices.erase(indices.begin()+i);
                numIndicesRemoved++;
            }
        }
        assert(numIndicesRemoved==3 || numIndicesRemoved==6);
        //Shift the values of vertex indices greater than the ones removed.
        {
            //Indices vector
            for(int i = indices.size()-1; i>=0; i--) {
                if(indices[i]>end) indices[i] -= numVertsRemoved;
            }
            //Indices map
            for(auto& elem : indicesMap) {
                if(elem.second.at(0)>end) {
                    for(int i = 0; i<elem.second.size(); i++) {
                        elem.second.at(i) -= numVertsRemoved;
                    }
                }
            }
        }
        //Remove appropriate vertices and element from 'indicesMap'.
        vertices.erase(vertices.begin()+beg, vertices.begin()+end+1);
        indicesMap.erase(iit);
    }
}

void Mesh::cleanup() {
    assert(built);
    built = false;
    if(glEBO) glDeleteBuffers(1, &glEBO);
    if(glVBO) glDeleteBuffers(1, &glVBO);
    if(glVAO) glDeleteVertexArrays(1, &glVAO);
    glEBO = glVBO = glVAO = 0;
}
void Mesh::setup()
{
    assert(!built);
    drawnIndexCount = static_cast<GLsizei>(indices.size());

    glVAO = VAO::build();
    glGenBuffers(1, &glVBO);
    glGenBuffers(1, &glEBO);
    assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(glVAO);
    glBindBuffer(GL_ARRAY_BUFFER, glVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    assert(glGetError() == GL_NO_ERROR);

    //Vertex Attributes
    GLsizei stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, texUV));
    assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(0);
    assert(glGetError() == GL_NO_ERROR);

    built = true;
}
