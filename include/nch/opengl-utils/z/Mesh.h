#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <nch/math-utils/box3.h>
#include "nch/opengl-utils/atlas.h"
#include "nch/opengl-utils/camera3d.h"
#include "nch/opengl-utils/geo.h"
#include "nch/opengl-utils/shader.h"

namespace nch { class Mesh {
public:
    struct MPoly {
        MPoly(const Poly& poly) {
            verts.reserve(poly.getNumVerts());
            for(int i = 0; i<poly.getNumVerts(); i++) {
                verts.push_back(poly.v(i));
            }
        }
        std::vector<Vertex> verts;
    };

    Mesh(const std::vector<Atlas*>& atlases);
    ~Mesh();

    void draw(nch::Shader* shader, nch::Camera3D* cam);
    int getInternalVerticesSize();
    int getInternalIndicesSize();
    bool isBuilt();

    void applyUpdates();
    void reset();
    void addPoly(const glm::ivec3& key, const Poly& poly);
    void addPoly(const Poly& poly);
    void remove(const glm::ivec3& key);

private:
    void setup();
    void cleanup();

    bool built = false;
    GLuint glVAO, glVBO, glEBO;
    GLsizei drawnIndexCount = 0;
    
    std::multimap<std::tuple<int, int, int>, MPoly> polyMap;
    std::multimap<std::tuple<int, int, int>, std::vector<GLuint>> indicesMap;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Atlas*> atlases;
}; }
