#pragma once
#include <glm/vec2.hpp>
#include <vector>
#include <string>
#include "Vertex.h"

class Poly {
public:
    enum PolyType {
        UNKNOWN, TRI, QUAD
    };

    Poly();
    Poly(const std::vector<Vertex>& verts);
    ~Poly();
    int getType() const;
    int getNumVerts() const;
    int vs() const;
    Vertex v(int idx) const;
    glm::vec3 norm() const;
    std::string toString() const;
    bool operator<(const Poly& other) const {
        int s = vs(), os = other.vs();
        if(s<os) return true;
        if(os<s) return false;

        for(int i = 0; i<s; i++) {
            if(v(i).pos.x<other.v(i).pos.x) return true;
            if(v(i).pos.y<other.v(i).pos.y) return true;
            if(v(i).pos.z<other.v(i).pos.z) return true;
        }
        return false;
    }

    void simplyTex(const glm::vec2& uv0, const glm::vec2& uv1);
    void rotate(const glm::vec3& center, const glm::vec3& xyzRot);
    void move(const glm::vec3& offset);
    void invertNorm();
    std::vector<Poly> split() const;

    Vertex& operator[](int idx) {
        assert(idx>=0 && idx<verts.size() && "Index out of range!");
        return verts[idx];
    }
    const Vertex& operator[](int idx) const {
        assert(idx>=0 && idx<verts.size() && "Index out of range!");
        return verts[idx];
    }

protected:
    void super_type(int type);
    void super_updateNormals();
    void super_updateColors();

    std::vector<Vertex> verts;
private:
    glm::vec3 normal;
    int type = PolyType::UNKNOWN;
};