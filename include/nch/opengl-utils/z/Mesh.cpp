#include "Mesh.h"
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include <nch/math-utils/chunkmath.h>
#include <set>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "nch/opengl-utils/geo.h"
#include "nch/opengl-utils/xbo.h"

using namespace nch;

Mesh::Mesh(){}
Mesh::Mesh(const std::vector<Atlas*>& atlases) {
    setAtlases(atlases);
}
Mesh::Mesh(Mesh&& other) noexcept:
built(other.built),
glVAO(other.glVAO),
glVBO(other.glVBO),
glEBO(other.glEBO),
drawnIndexCount(other.drawnIndexCount),
chkPos(other.chkPos),
subPos(other.subPos),
scale(other.scale),
rotation(other.rotation),
centerOfRotation(other.centerOfRotation),
skybox(other.skybox),
polyMap(std::move(other.polyMap)),
indicesMap(std::move(other.indicesMap)),
vertices(std::move(other.vertices)),
indices(std::move(other.indices)),
atlases(other.atlases),
frustumCache(other.frustumCache)
{
    //Invalidate source's OpenGL handles so destructor doesn't delete them
    other.glVAO = 0;
    other.glVBO = 0;
    other.glEBO = 0;
    other.built = false;
}
Mesh::~Mesh() {
    reset();
}

void Mesh::draw(Shader* shader, Camera3D* cam) const
{
    assert(built);
    if(!built) return;
    shader->useProgram();

    //Set camera draw-from position for render position
    Vec3i64 fChk = cam->getRegPos()-chkPos;
    Vec3f fSub = -subPos;
    if(!skybox) {
        cam->drawFromPos(shader, (fChk*32).toFloat()+fSub);
    } else {
        //For skybox, render with camera at origin (no translation)
        cam->drawSkybox(shader);
    }
    //Create model matrix with scale, rotation, and center of rotation
    shader->setModelMatrix(scale, rotation, centerOfRotation);

    if(atlases.size()!=0) {
        unsigned int numDiffuse = 0;
        unsigned int numSpecular = 0;
        for(size_t i = 0; i<atlases.size(); i++) {
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
    }

    //Paged atlases carry the page (array layer) in the integer part of u; single-page (and absent) atlases
    //keep raw pass-through UVs so REPEAT-dependent meshes (e.g. sphere seams with u slightly >1) still work.
    bool atlasPaged = atlases.size()!=0 && atlases[0]->getPageCount()>1;
    GLint atlasPagedLoc = glGetUniformLocation(shader->getID(), "atlasPaged");
    if(atlasPagedLoc!=-1) glUniform1i(atlasPagedLoc, atlasPaged ? 1 : 0);

    VAO::bind(glVAO);
    assert(glIsVertexArray(glVAO) == GL_TRUE);
    glDrawElements(GL_TRIANGLES, drawnIndexCount, GL_UNSIGNED_INT, nullptr);
    VAO::unbind();

    if(atlases.size()!=0) {
        for(size_t i = 0; i<atlases.size(); i++) {
            atlases[i]->unbind();
        }
    }
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
bool Mesh::isChunkInFrustum(Camera3D* cam)
{
    //Check if inputs changed since last call; if not, return cached result
    Vec3i64 camRegPos = cam->getRegPos();
    Vec3f camSubPos = cam->getSubPos();
    Vec3f camRot = cam->getRot();
    Vec3f camPerspOffset = cam->getPerspectiveOffset();
    float camPerspDir = cam->getPerspectiveDir();
    if(frustumCache.valid
    && frustumCache.meshChkPos==chkPos && frustumCache.meshSubPos==subPos && frustumCache.meshScale==scale
    && frustumCache.camRegPos==camRegPos && frustumCache.camSubPos==camSubPos && frustumCache.camRot==camRot
    && frustumCache.camPerspOffset==camPerspOffset && frustumCache.camPerspDir==camPerspDir) {
        return frustumCache.lastResult;
    }

    //Compute camera offset matching how draw() positions this mesh (tile-world space, no scale)
    Vec3f offset = ((camRegPos-chkPos)*32).toFloat()-subPos;
    glm::mat4 mvp = cam->getCMatrixForOffset(offset);

    //Extract and normalize frustum planes (Gribb/Hartmann)
    glm::vec4 planes[6];
    planes[0] = {mvp[0][3]+mvp[0][0], mvp[1][3]+mvp[1][0], mvp[2][3]+mvp[2][0], mvp[3][3]+mvp[3][0]}; // Left
    planes[1] = {mvp[0][3]-mvp[0][0], mvp[1][3]-mvp[1][0], mvp[2][3]-mvp[2][0], mvp[3][3]-mvp[3][0]}; // Right
    planes[2] = {mvp[0][3]+mvp[0][1], mvp[1][3]+mvp[1][1], mvp[2][3]+mvp[2][1], mvp[3][3]+mvp[3][1]}; // Bottom
    planes[3] = {mvp[0][3]-mvp[0][1], mvp[1][3]-mvp[1][1], mvp[2][3]-mvp[2][1], mvp[3][3]-mvp[3][1]}; // Top
    planes[4] = {mvp[0][3]+mvp[0][2], mvp[1][3]+mvp[1][2], mvp[2][3]+mvp[2][2], mvp[3][3]+mvp[3][2]}; // Near
    planes[5] = {mvp[0][3]-mvp[0][2], mvp[1][3]-mvp[1][2], mvp[2][3]-mvp[2][2], mvp[3][3]-mvp[3][2]}; // Far
    for(auto& plane : planes) {
        plane /= glm::length(glm::vec3(plane));
    }

    //Bounding sphere in tile-world space: scaled [0,32*scale]^3 box centered at (16*scale, 16*scale, 16*scale)
    const glm::vec3 center(16.0f*scale.x, 16.0f*scale.y, 16.0f*scale.z);
    const float radius = glm::length(center);

    //Sphere-plane test: outside if behind any plane by more than radius
    bool result = true;
    for(int i = 0; i<6; i++) {
        if(glm::dot(glm::vec3(planes[i]), center)+planes[i].w < -radius) { result = false; break; }
    }

    //Update cache
    frustumCache.meshChkPos = chkPos;
    frustumCache.meshSubPos = subPos;
    frustumCache.meshScale = scale;
    frustumCache.camRegPos = camRegPos;
    frustumCache.camSubPos = camSubPos;
    frustumCache.camRot = camRot;
    frustumCache.camPerspOffset = camPerspOffset;
    frustumCache.camPerspDir = camPerspDir;
    frustumCache.lastResult = result;
    frustumCache.valid = true;
    return result;
}
bool Mesh::isChunkInFrustum(Camera3D* cam, const std::vector<glm::vec4>& cullingPlanes)
{
    //Offset from camera to chunk center in tile-world space
    Vec3f offset = ((cam->getRegPos()-chkPos)*32).toFloat()-subPos;

    //Transform the chunk-center through the SAME model matrix draw() applies (scale + rotation about
    //centerOfRotation; see Shader::setModelMatrix), then shift into camera-relative cull space by -offset.
    //For unrotated, origin-centered meshes (terrain) this collapses to (16*scale - offset). For a rotated
    //PhysStructure, whose chunks rotate about the structure origin (far from the chunk's own center), the
    //center swings accordingly — testing the un-rotated location wrongly culled visible chunks.
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(centerOfRotation.x, centerOfRotation.y, centerOfRotation.z));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale.x, scale.y, scale.z));
    model = glm::translate(model, glm::vec3(-centerOfRotation.x, -centerOfRotation.y, -centerOfRotation.z));
    glm::vec3 worldCenter = glm::vec3(model*glm::vec4(16.0f, 16.0f, 16.0f, 1.0f));

    //Bounding sphere center translated into camera-relative space (rotation preserves the radius).
    const glm::vec3 center(worldCenter.x - offset.x, worldCenter.y - offset.y, worldCenter.z - offset.z);
    const float radius = glm::length(glm::vec3(16.0f*scale.x, 16.0f*scale.y, 16.0f*scale.z));

    for(int i = 0; i<6; i++) {
        if(glm::dot(glm::vec3(cullingPlanes[i]), center)+cullingPlanes[i].w < -radius) return false;
    }
    return true;
}
Vec3f Mesh::getGeometricCenter() {
    if(vertices.size()==0) return {0.0f, 0.0f, 0.0f};
    Vec3f sum = {0.0f, 0.0f, 0.0f};
    for(const Vertex& v : vertices) {
        sum.x += v.pos.x;
        sum.y += v.pos.y;
        sum.z += v.pos.z;
    }
    float count = static_cast<float>(vertices.size());
    return {sum.x/count, sum.y/count, sum.z/count};
}
std::vector<Poly> Mesh::getPolysAt(glm::ivec3 key) {
    std::vector<Poly> ret;
    
    auto range = polyMap.equal_range({key.x, key.y, key.z});
    for (auto it = range.first; it!=range.second; it++) {
        ret.push_back(it->second.verts);
    }

    return ret;
}
Vec3i64 Mesh::getChunkPos() const {
    return chkPos;
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
    //If key is valid -> check if this MPoly already exists at this key
    MPoly newPoly(poly);
    if(key.x>=0 && key.y>=0 && key.z>=0) {
        auto range = polyMap.equal_range({key.x, key.y, key.z});
        for(auto it = range.first; it != range.second; ++it) {
            if(it->second == newPoly) {
                //Duplicate found, don't add
                return;
            }
        }
    }

    //Add the poly to the 'polyMap'
    polyMap.insert({{key.x, key.y, key.z}, newPoly});

    //Add vertices and indices to 'vertices', 'indices', and 'indicesMap'
    GLuint vs = vertices.size();
    if(poly.getNumVerts()==3) {
        for(int i = 0; i<3; i++) vertices.push_back(poly.v(i));
        indices.insert   (indices.end(),          {vs+0, vs+1, vs+2});
        indicesMap.insert({{key.x, key.y, key.z}, {vs+0, vs+1, vs+2}});
    }
    else if (poly.getNumVerts()==4) {
        for(int i = 0; i<4; i++) vertices.push_back(poly.v(i));
        
        /*
            Written by Claude
            //Choose diagonal based on light: put it through the two brighter corners so
            //the gradient spreads across the full face regardless of which corner is lit.
            One side effect to be aware of: the indicesMap now stores different index sets per quad depending on light values, which means Mesh::remove() needs to match the same stored set.
            Since light values are fixed per-build and removal uses the stored map keys, this should work correctly — applyUpdates() rebuilds from scratch anyway when light changes.

            Old inserting:
            indices.insert   (indices.end(),          {vs+0, vs+1, vs+2, vs+0, vs+2, vs+3});
            indicesMap.insert({{key.x, key.y, key.z}, {vs+0, vs+1, vs+2, vs+0, vs+2, vs+3}});
        */
        float l02 = glm::length(poly.v(0).light) + glm::length(poly.v(2).light);
        float l13 = glm::length(poly.v(1).light) + glm::length(poly.v(3).light);
        std::vector<GLuint> order = (l13 > l02)
            ? std::vector<GLuint>{vs+0, vs+1, vs+3, vs+1, vs+2, vs+3}
            : std::vector<GLuint>{vs+0, vs+1, vs+2, vs+0, vs+2, vs+3};
        
        indices.insert   (indices.end(), order.begin(), order.end());
        indicesMap.insert({{key.x, key.y, key.z}, order});
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

void Mesh::setAtlases(const std::vector<Atlas*>& atlases) {
    Mesh::atlases = atlases;
}
void Mesh::setPos(Vec3i64 chkPos, Vec3f subPos) {
    Mesh::subPos = subPos;
    Mesh::chkPos = chkPos;
}
void Mesh::setPos(Vec3d approxPos) {
    chkPos = nch::chunked3D(approxPos);
    subPos = nch::subbed3D(approxPos).toFloat();
}
void Mesh::setPos(Vec3f approxPos) {
    chkPos = nch::chunked3F(approxPos);
    subPos = nch::subbed3F(approxPos);
}
void Mesh::setScale(Vec3f scale) {
    Mesh::scale = scale;
}
void Mesh::setScale(float uniformScale) {
    Mesh::scale = {uniformScale, uniformScale, uniformScale};
}
void Mesh::setRotation(Vec3f rotation) {
    Mesh::rotation = rotation;
}
void Mesh::setCenterOfRotation(Vec3f center) {
    Mesh::centerOfRotation = center;
}
void Mesh::setCenterOfRotation() {
    setCenterOfRotation(getGeometricCenter());
}
void Mesh::setSkybox(bool skybox) {
    Mesh::skybox = skybox;
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
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, light));
    assert(glGetError() == GL_NO_ERROR);

    glBindVertexArray(0);
    assert(glGetError() == GL_NO_ERROR);

    built = true;
}
