#include "MeshTree.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <nch/math-utils/chunkmath.h>
#include <nch/math-utils/consts.h>
using namespace nch;

MeshTree::MeshTree() {
    base.tree = this;
}
MeshTree::MeshTree(const std::string& baseName) {
    base.objName = baseName;
    base.tree = this;
    nodes[baseName] = &base;

    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}

void MeshTree::reset(const std::string& baseName) {
    //Delete all nodes except base
    for(auto& pair : nodes) {
        if(pair.second != &base) delete pair.second;
    }
    nodes.clear();
    base.children.clear();
    base.objName = baseName;
    base.tree = this;
    base.absoluteOrigin = {0,0,0};
    base.origin = {0,0,0};
    base.rot = {0,0,0};
    scale = {1,1,1};
    modelOffset = {0,0,0};
    nodes[baseName] = &base;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}

Vec3d MeshTree::getPos() const {
    return pos;
}
Vec3d MeshTree::getScale() const {
    return scale;
}
Vec3d MeshTree::getModelOffset() const {
    return modelOffset;
}
glm::dmat3 MeshTree::getBaseRotation() const {
    return baseRotMat;
}
bool MeshTree::isBaseRotationActive() const {
    return baseRotActive;
}
void MeshTree::setBaseRotation(const glm::dmat3& m) {
    baseRotMat = m;
    //Treat near-identity as inactive so the cheap render path (and exact legacy behavior) is kept at up=+Y.
    glm::dmat3 d = m-glm::dmat3(1.0);
    double err = 0;
    for(int c = 0; c<3; c++) for(int r = 0; r<3; r++) err += d[c][r]*d[c][r];
    baseRotActive = err>1e-12;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}

void MeshTree::setBaseOrigin(Vec3d absoluteOrigin) {
    //Both, and to the same value: 'absoluteOrigin' is what the model build shifts the geometry against
    //and what children measure their attachment points from, while 'origin' is the offset from the tree
    //anchor. Setting both leaves the geometry exactly where it was and moves only the pivot onto it.
    base.absoluteOrigin = absoluteOrigin;
    base.origin = absoluteOrigin;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}
void MeshTree::setPos(Vec3d p) {
    pos = p;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}
void MeshTree::setScale(Vec3d s) {
    scale = s;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}
void MeshTree::setModelOffset(Vec3d offset) {
    modelOffset = offset;
    base.updateTransforms(pos+modelOffset*scale, glm::dmat3(1.0));
}

MeshTree::~MeshTree() {
    for(auto& pair : nodes) {
        if(pair.second != &base) delete pair.second;
    }
}

Vec3d MeshTreeNode::getOrigin() const {
    return origin;
}
Vec3d MeshTreeNode::getAbsoluteOrigin() const {
    return absoluteOrigin;
}
Vec3d MeshTreeNode::getWorldPos() const {
    return worldPosRender; //rendered (base-rotated) position; equals worldPos when no base rotation is active
}
glm::dmat3 MeshTreeNode::getWorldRotation() const {
    //The same outer transform updateTransforms renders with: the base rotation wraps the assembled pose.
    if(tree && tree->isBaseRotationActive()) return tree->getBaseRotation()*worldRotMat;
    return worldRotMat;
}

void MeshTreeNode::addChild(const std::string& name, Vec3d absOrigin, Vec3d relativeRot) {
    MeshTreeNode* child = new MeshTreeNode();
    child->objName = name;
    child->absoluteOrigin = absOrigin;
    child->origin = absOrigin-absoluteOrigin;
    child->rot = relativeRot;
    child->parent = this;
    child->tree = tree;
    children.push_back(child);
    if(tree) tree->nodes[name] = child;
    child->updateTransforms(worldPos, worldRotMat);
}

Vec3d MeshTreeNode::getRotation() const { return rot; }

void MeshTreeNode::rotate(Vec3d relativeRot) {
    rot += relativeRot;
    Vec3d pPos = parent ? parent->worldPos : (tree ? tree->getPos() : Vec3d(0,0,0));
    glm::dmat3 pRotMat = parent ? parent->worldRotMat : glm::dmat3(1.0);
    updateTransforms(pPos, pRotMat);
}

void MeshTreeNode::setRotation(Vec3d absoluteRot) {
    rot = absoluteRot;
    Vec3d pPos = parent ? parent->worldPos : (tree ? tree->getPos() : Vec3d(0,0,0));
    glm::dmat3 pRotMat = parent ? parent->worldRotMat : glm::dmat3(1.0);
    updateTransforms(pPos, pRotMat);
}

void MeshTreeNode::updateTransforms(Vec3d parentWorldPos, const glm::dmat3& parentWorldRotMat) {
    Vec3d s = tree ? tree->getScale() : nch::Vec3d(1,1,1);
    //Assemble the pose in the model's natural (base-identity) frame. Rotations compose as MATRICES
    //(parent*child) so a child turns INSIDE its parent's frame: e.g. a steering wheel's Z spin stays about
    //the axle its Y-yawed parent points down. Euler-ADDing instead would force the sum through a fixed
    //Rz*Ry*Rx, which only agrees when the child's axis is inner to the parent's (X inner, Z outer) — a
    //Z child under a Y parent would spin about the WORLD z and tumble.
    Vec3d os = origin*s;
    glm::dvec3 o = parentWorldRotMat*glm::dvec3(os.x, os.y, os.z);
    worldPos = parentWorldPos + Vec3d(o.x, o.y, o.z);
    worldRotMat = parentWorldRotMat*eulerZYXMat(rot);
    worldRot = matToEulerZYXdeg(worldRotMat);

    //Render transform: if the tree carries a base rotation (e.g. aligning +Y to the player's up axis on a
    //wall), rotate the assembled pose rigidly about the base anchor. Otherwise render the local pose as-is,
    //skipping the transform entirely.
    if(tree && tree->isBaseRotationActive()) {
        glm::dmat3 B = tree->getBaseRotation();
        //Pivot about the planted point the model is positioned at (the feet/contact point = tree pos), NOT
        //the model-offset base node — otherwise modelOffset (e.g. the human's +1.2 Y lift) stays unrotated
        //and the whole body swings off by (I-B)*modelOffset. Pivoting here rotates modelOffset with the model.
        Vec3d anchor = tree->getPos();
        glm::dvec3 rel(worldPos.x-anchor.x, worldPos.y-anchor.y, worldPos.z-anchor.z);
        glm::dvec3 rp = glm::dvec3(anchor.x, anchor.y, anchor.z)+B*rel;
        worldPosRender = Vec3d(rp.x, rp.y, rp.z);
        Vec3d renderRot = matToEulerZYXdeg(B*worldRotMat);
        mesh.setPos(nch::chunked3D(worldPosRender), nch::subbed3D(worldPosRender).toFloat());
        mesh.setRotation(renderRot.toFloat());
    } else {
        worldPosRender = worldPos;
        mesh.setPos(nch::chunked3D(worldPos), nch::subbed3D(worldPos).toFloat());
        mesh.setRotation(worldRot.toFloat());
    }
    mesh.setScale(s.toFloat());

    for(MeshTreeNode* child : children) {
        child->updateTransforms(worldPos, worldRotMat); //recurse in the LOCAL (base-identity) frame
    }
}

glm::dmat3 MeshTreeNode::eulerZYXMat(Vec3d eulerDeg) {
    glm::dmat4 m(1.0);
    m = glm::rotate(m, glm::radians(eulerDeg.z), glm::dvec3(0,0,1));
    m = glm::rotate(m, glm::radians(eulerDeg.y), glm::dvec3(0,1,0));
    m = glm::rotate(m, glm::radians(eulerDeg.x), glm::dvec3(1,0,0));
    return glm::dmat3(m);
}
Vec3d MeshTreeNode::matToEulerZYXdeg(const glm::dmat3& m) {
    //glm is column-major: m[col][row], so math M[r][c]=m[c][r].
    double M00=m[0][0], M10=m[0][1], M20=m[0][2], M21=m[1][2], M22=m[2][2];
    double y = std::atan2(-M20, std::sqrt(M00*M00 + M10*M10));
    double x, z;
    if(std::sqrt(M00*M00 + M10*M10) > 1e-9) {
        z = std::atan2(M10, M00);
        x = std::atan2(M21, M22);
    } else {
        //Gimbal lock (pitch ~ ±90°): fold the X/Z rotation together; pick z=0. With z=0, M12=-sin(x) and
        //M11=cos(x) at BOTH poles — M01 (the old term) flips sign at -90°, mirroring x there.
        z = 0;
        x = std::atan2(-m[2][1], m[1][1]);
    }
    return Vec3d(x, y, z)*nch::deg_rad;
}
