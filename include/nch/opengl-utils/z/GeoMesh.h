#pragma once
#include <GL/glew.h>
#include <vector>
#include "nch/opengl-utils/z/geo/Vertex.h"

namespace nch { class Shader; class Camera3D; }

/**
 * @brief Batched colored line/poly mesh — the untextured sibling of nch::Mesh, for debug/overlay
 * geometry (box wireframes, graph adjacency lines, translucent fills).
 *
 * Usage mirrors nch::Mesh: stage geometry with the add* calls, upload once with applyUpdates(),
 * then draw() each frame; rebuild via reset() + re-stage. Staged positions are camera-relative
 * world space (the space Camera3D::drawFromPos expects) — the caller applies any camera-region
 * subtraction or structure transform before staging, and owns the rebuild/dirty policy.
 */

namespace nch { class GeoMesh {
public:
    ~GeoMesh();

    //The 8 corners of an axis-aligned box, indexed xi*4 + yi*2 + zi — the convention every
    //corners[8] parameter below expects. Callers transforming each corner (rotated structures)
    //build the array themselves in the same order.
    static void boxCorners(const Vec3f& min, const Vec3f& size, Vec3f out[8]);

    void addLine(const Vec3f& a, const Vec3f& b, const Vec3f& color);
    void addLine(const Vec3f& a, const Vec3f& b, const Vec3f& colorA, const Vec3f& colorB);
    //Arrow from a to b: the shaft line plus a 4-fin head at b (fins span both perpendicular axes
    //so the head reads from any view angle; sized ~30%/15% of the arrow length).
    void addArrow(const Vec3f& a, const Vec3f& b, const Vec3f& color);
    void addArrow(const Vec3f& a, const Vec3f& b, const Vec3f& colorA, const Vec3f& colorB);
    void addTri(const Vec3f& a, const Vec3f& b, const Vec3f& c, const Vec3f& color);
    void addQuad(const Vec3f corners[4], const Vec3f& color);
    void addBoxOutline(const Vec3f corners[8], const Vec3f& color);
    void addBoxOutline(const Vec3f& min, const Vec3f& size, const Vec3f& color);
    void addBoxFill(const Vec3f corners[8], const Vec3f& color);

    //Upload the staged vertices into fresh GL buffers (requires a current GL context).
    void applyUpdates();
    //Clear staged vertices and release the GL buffers.
    void reset();
    bool isBuilt() const;

    /**
     * @brief Draw fills then lines with debug-overlay state: no lighting, blending on, depth test
     * and face culling off (culling and lighting are restored; depth test is left off, matching the
     * debug passes this was extracted from).
     * @param triAlpha alphaMod for the fill pass (e.g. a small pulsing value keeps lines readable).
     * @param lineAlpha alphaMod for the line pass.
     */
    void draw(nch::Shader* sdr, nch::Camera3D* cam, float triAlpha = 1.0f, float lineAlpha = 1.0f) const;

private:
    //Build a VAO/VBO pair from verts and link the standard nch::Vertex attribs; count 0 if empty.
    static void upload(std::vector<nch::Vertex>& verts, GLuint& vao, GLuint& vbo, GLsizei& count);
    //Destroy the GL buffers only (staged vertices untouched).
    void releaseBuffers();

    std::vector<nch::Vertex> lineVerts;
    std::vector<nch::Vertex> triVerts;
    GLuint lineVAO = 0, lineVBO = 0;
    GLsizei lineVtxCount = 0;
    GLuint triVAO = 0, triVBO = 0;
    GLsizei triVtxCount = 0;
}; }
