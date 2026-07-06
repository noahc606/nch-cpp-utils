#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
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
    static void boxCorners(const glm::vec3& min, const glm::vec3& size, glm::vec3 out[8]);

    void addLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color);
    void addLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& colorA, const glm::vec3& colorB);
    //Arrow from a to b: the shaft line plus a 4-fin head at b (fins span both perpendicular axes
    //so the head reads from any view angle; sized ~30%/15% of the arrow length).
    void addArrow(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color);
    void addArrow(const glm::vec3& a, const glm::vec3& b, const glm::vec3& colorA, const glm::vec3& colorB);
    void addTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color);
    void addQuad(const glm::vec3 corners[4], const glm::vec3& color);
    void addBoxOutline(const glm::vec3 corners[8], const glm::vec3& color);
    void addBoxOutline(const glm::vec3& min, const glm::vec3& size, const glm::vec3& color);
    void addBoxFill(const glm::vec3 corners[8], const glm::vec3& color);

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
