#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "nch/math-utils/vec3.h"
#include "nch/opengl-utils/mesh.h"

namespace nch {

class MeshTree;

class MeshTreeNode {
public:
    nch::Vec3d getOrigin() const;
    nch::Vec3d getAbsoluteOrigin() const;
    nch::Vec3d getWorldPos() const; //the rendered (base-rotated) world position
    glm::dmat3 getWorldRotation() const; //the rendered (base-rotated) world rotation; its columns are the node's own axes
    nch::Vec3d getRotation() const; //rotation relative to the parent (ZYX Euler, deg)

    void addChild(const std::string& name, nch::Vec3d absoluteOrigin, nch::Vec3d relativeRot);
    void rotate(nch::Vec3d relativeRot);
    void setRotation(nch::Vec3d absoluteRot);

    nch::Mesh mesh;
    std::string objName = "";
    std::vector<MeshTreeNode*> children;
private:
    friend class MeshTree;
    //The hierarchy is composed in the model's natural (base-identity) frame as MATRICES (parent*child), so
    //a child's rotation is applied INSIDE its parent's frame. The tree's base rotation is then applied as an
    //OUTER rigid transform (about the base anchor) to each node's rendered pos/rot — so a posed mob can be
    //tilted to stand on a wall WITHOUT changing the internal animation composition.
    void updateTransforms(nch::Vec3d parentWorldPos, const glm::dmat3& parentWorldRotMat);

    /**
     * @brief ZYX-Euler (degrees) -> rotation matrix, matching nch::Mesh::setRotation (Rz*Ry*Rx).
     */
    static glm::dmat3 eulerZYXMat(nch::Vec3d eulerDeg);
    /**
     * @brief Inverse of eulerZYXMat: rotation matrix -> ZYX-Euler (degrees), so a composed rotation can be handed to nch::Mesh.
     */
    static nch::Vec3d matToEulerZYXdeg(const glm::dmat3& m);

    nch::Vec3d absoluteOrigin = {0,0,0};       //Position in model space (for mesh centering)
    nch::Vec3d origin = {0,0,0};               //Attachment point in parent's local space (computed)
    nch::Vec3d rot = {0,0,0};                  //Rotation relative to parent (ZYX Euler, deg)
    nch::Vec3d worldPos = {0,0,0};             //Base-identity (local) world pos — used to propagate the hierarchy.
    glm::dmat3 worldRotMat = glm::dmat3(1.0);  //Base-identity (local) world rot — the composed parent*child matrix.
    nch::Vec3d worldRot = {0,0,0};             //worldRotMat as ZYX Euler (deg), i.e. what nch::Mesh takes.
    nch::Vec3d worldPosRender = {0,0,0}; //Rendered world pos after the tree base rotation (for shadows/icons).
    MeshTreeNode* parent = nullptr;
    MeshTree* tree = nullptr;
};

class MeshTree {
public:
    MeshTree();
    MeshTree(const std::string& baseName);
    ~MeshTree();

    void reset(const std::string& baseName);
    /**
     * @brief Give the BASE node an origin in model space, the way every child already declares one.
     *
     * A node turns about its own origin (its geometry is shifted by -origin at build time), and the
     * base's was hardwired to (0,0,0) — so a base carrying a rotation pivoted at the model's corner
     * instead of wherever the part actually turns. Call between reset() and adding children: a child's
     * attachment point is computed against the base's origin at addChild time.
     */
    void setBaseOrigin(nch::Vec3d absoluteOrigin);

    nch::Vec3d getPos() const;
    nch::Vec3d getScale() const;
    nch::Vec3d getModelOffset() const;
    glm::dmat3 getBaseRotation() const;
    bool isBaseRotationActive() const;
    void setPos(nch::Vec3d pos);
    void setScale(nch::Vec3d scale);
    void setModelOffset(nch::Vec3d offset);
    //Root orientation applied to the whole tree (e.g. align the model's +Y with the player's up axis when
    //walking on a wall). Identity by default — when identity the render path is skipped entirely, so
    //ordinary mobs are byte-for-byte unchanged.
    void setBaseRotation(const glm::dmat3& m);

    MeshTreeNode base;
    std::unordered_map<std::string, MeshTreeNode*> nodes;
private:
    nch::Vec3d pos = {0,0,0};
    nch::Vec3d modelOffset = {0, 0, 0};
    nch::Vec3d scale = {1,1,1};
    glm::dmat3 baseRotMat = glm::dmat3(1.0);
    bool baseRotActive = false; //true once a non-identity base rotation is set; gates the render transform.
};

}
