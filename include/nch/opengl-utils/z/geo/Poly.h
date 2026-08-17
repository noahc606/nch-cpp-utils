#pragma once
#include <vector>
#include <string>
#include "Vertex.h"

namespace nch { class Poly {
public:
    enum PolyType {
        UNKNOWN, TRI, QUAD
    };

    Poly();
    Poly(const std::vector<Vertex>& verts, float expansion = 0.0f);
    ~Poly();
    int getType() const;
    int getNumVerts() const;
    int vs() const;
    Vertex v(int idx) const;
    Vec3f norm() const;
    std::string toString() const;
    bool usingManualNormals() const;
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

    void simplyTex(Vec2f uv0, Vec2f uv1);
    void expand(float amount);
    void rotate(const Vec3f& center, const Vec3f& xyzRotRad);
    void move(const Vec3f& offset);
    void invertNorm();
    void useManualNormals(bool useManNormals);
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
    Vec3f normal;
    bool manualNormals = false;
private:
    int type = PolyType::UNKNOWN;
}; }
