#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "nch/math-utils/vec3.h"
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

        bool operator==(const MPoly& other) const {
            if(verts.size() != other.verts.size()) return false;
            for(size_t i = 0; i<verts.size(); i++) {
                if(!glm::all(glm::epsilonEqual(verts[i].pos, other.verts[i].pos, 0.0001f))) return false;
                if(!glm::all(glm::epsilonEqual(verts[i].normal, other.verts[i].normal, 0.0001f))) return false;
            }
            return true;
        }
    };

    Mesh();
    Mesh(const std::vector<Atlas*>& atlases);
    Mesh(Mesh& other);
    ~Mesh();

    void draw(nch::Shader* shader, nch::Camera3D* cam);
    int getInternalVerticesSize();
    int getInternalIndicesSize();
    bool isBuilt();
    Vec3f getGeometricCenter();
    std::vector<Poly> getPolysAt(glm::ivec3 key);

    void applyUpdates();
    void addPoly(const glm::ivec3& key, const Poly& poly);
    void addPoly(const Poly& poly);
    void remove(const glm::ivec3& key);
    void reset();

    void setAtlases(const std::vector<Atlas*>& atlases);
    void setPos(Vec3i64 chkPos, Vec3f subPos);
    void setPos(Vec3f approxPos);
    void setScale(Vec3f scale);
    void setScale(float uniformScale);
    void setRotation(Vec3f rotation);
    void setCenterOfRotation(Vec3f center);
    void setSkybox(bool skybox);

private:
    void setup();
    void cleanup();

    bool built = false;
    GLuint glVAO, glVBO, glEBO;
    GLsizei drawnIndexCount = 0;
    nch::Vec3i64 chkPos = {0};
    nch::Vec3f subPos = {0};
    nch::Vec3f scale = {1.0f, 1.0f, 1.0f};
    nch::Vec3f rotation = {0.0f, 0.0f, 0.0f};
    nch::Vec3f centerOfRotation = {0.0f, 0.0f, 0.0f};
    bool skybox = false;

    std::multimap<std::tuple<int, int, int>, MPoly> polyMap;
    std::multimap<std::tuple<int, int, int>, std::vector<GLuint>> indicesMap;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Atlas*> atlases;
}; }
