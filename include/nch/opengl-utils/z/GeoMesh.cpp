#include "GeoMesh.h"
#include "nch/opengl-utils/camera3d.h"
#include "nch/opengl-utils/shader.h"
#include "nch/opengl-utils/z/BufferObj.h"

using namespace nch;

GeoMesh::~GeoMesh() {
    releaseBuffers();
}

void GeoMesh::boxCorners(const glm::vec3& min, const glm::vec3& size, glm::vec3 out[8]) {
    for(int xi = 0; xi<2; xi++)
    for(int yi = 0; yi<2; yi++)
    for(int zi = 0; zi<2; zi++) {
        out[xi*4+yi*2+zi] = glm::vec3(
            min.x+(xi ? size.x : 0.0f),
            min.y+(yi ? size.y : 0.0f),
            min.z+(zi ? size.z : 0.0f)
        );
    }
}

void GeoMesh::addLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color) {
    addLine(a, b, color, color);
}
void GeoMesh::addLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& colorA, const glm::vec3& colorB) {
    lineVerts.emplace_back(a, colorA);
    lineVerts.emplace_back(b, colorB);
}
void GeoMesh::addTri(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color) {
    triVerts.emplace_back(a, color);
    triVerts.emplace_back(b, color);
    triVerts.emplace_back(c, color);
}
void GeoMesh::addQuad(const glm::vec3 corners[4], const glm::vec3& color) {
    addTri(corners[0], corners[1], corners[2], color);
    addTri(corners[0], corners[2], corners[3], color);
}
void GeoMesh::addBoxOutline(const glm::vec3 corners[8], const glm::vec3& color) {
    //12 edges of a box over its 8 corners (corner index = xi*4 + yi*2 + zi).
    static const int edges[12][2] = {
        {0,4},{1,5},{2,6},{3,7}, {0,2},{1,3},{4,6},{5,7}, {0,1},{2,3},{4,5},{6,7}
    };
    for(int e = 0; e<12; e++) {
        addLine(corners[edges[e][0]], corners[edges[e][1]], color);
    }
}
void GeoMesh::addBoxOutline(const glm::vec3& min, const glm::vec3& size, const glm::vec3& color) {
    glm::vec3 c[8];
    boxCorners(min, size, c);
    addBoxOutline(c, color);
}
void GeoMesh::addBoxFill(const glm::vec3 corners[8], const glm::vec3& color) {
    static const int faceCorners[6][4] = {
        {0, 2, 3, 1}, //-X
        {4, 6, 7, 5}, //+X
        {0, 4, 5, 1}, //-Y
        {2, 6, 7, 3}, //+Y
        {0, 4, 6, 2}, //-Z
        {1, 5, 7, 3}, //+Z
    };
    for(int f = 0; f<6; f++) {
        glm::vec3 q[4];
        for(int k = 0; k<4; k++) q[k] = corners[faceCorners[f][k]];
        addQuad(q, color);
    }
}

void GeoMesh::applyUpdates() {
    releaseBuffers();
    upload(lineVerts, lineVAO, lineVBO, lineVtxCount);
    upload(triVerts, triVAO, triVBO, triVtxCount);
}
void GeoMesh::reset() {
    releaseBuffers();
    lineVerts.clear();
    triVerts.clear();
}
bool GeoMesh::isBuilt() const {
    return lineVtxCount>0 || triVtxCount>0;
}

void GeoMesh::draw(Shader* sdr, Camera3D* cam, float triAlpha, float lineAlpha) const {
    if(lineVtxCount==0 && triVtxCount==0) return;

    cam->drawFromPos(sdr);
    sdr->resetModelMatrix();
    glUniform1i(sdr->getUniformLoc("useLightEffects"), 0);
    glUniform1i(sdr->getUniformLoc("isLightSource"), 0);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if(triVtxCount>0) {
        glUniform1f(sdr->getUniformLoc("alphaMod"), triAlpha);
        VAO::bind(triVAO);
        VBO::bind(triVBO);
        glDrawArrays(GL_TRIANGLES, 0, triVtxCount);
    }
    if(lineVtxCount>0) {
        glUniform1f(sdr->getUniformLoc("alphaMod"), lineAlpha);
        VAO::bind(lineVAO);
        VBO::bind(lineVBO);
        glDrawArrays(GL_LINES, 0, lineVtxCount);
    }

    glUniform1f(sdr->getUniformLoc("alphaMod"), 1.0f);
    glEnable(GL_CULL_FACE);
    glUniform1i(sdr->getUniformLoc("useLightEffects"), 1);
}

void GeoMesh::upload(std::vector<Vertex>& verts, GLuint& vao, GLuint& vbo, GLsizei& count) {
    if(verts.empty()) { count = 0; return; }
    vao = VAO::build();
    VAO::bind(vao);
    vbo = VBO::build(verts);
    VAO::linkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
    VAO::linkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3*sizeof(float)));
    VAO::linkAttrib(vbo, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(6*sizeof(float)));
    VAO::linkAttrib(vbo, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(9*sizeof(float)));
    count = (GLsizei)verts.size();
}
void GeoMesh::releaseBuffers() {
    if(lineVBO) { VBO::destroy(lineVBO); lineVBO = 0; }
    if(lineVAO) { VAO::destroy(lineVAO); lineVAO = 0; }
    lineVtxCount = 0;
    if(triVBO) { VBO::destroy(triVBO); triVBO = 0; }
    if(triVAO) { VAO::destroy(triVAO); triVAO = 0; }
    triVtxCount = 0;
}
