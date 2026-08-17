#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "nch/math-utils/vec2.h"
#include "nch/math-utils/vec3.h"
#include "nch/math-utils/vec4.h"
#include "nch/opengl-utils/shader.h"

namespace nch { class Camera3D {
public:
    //Note: Uses RHR for +X/+Y/+Z.
    //XZ is the horizontal plane, and we assign +Z to north and -Z to south, thus -X for EAST and +X for WEST is correct.
    enum Direction {
        UNKNOWN = -1,
        EAST,  // -X
        WEST,  // +X
        DOWN,  // -Y
        UP,    // +Y
        SOUTH, // -Z
        NORTH, // +Z
    };

    Camera3D();
    Camera3D(SDL_Window* win);
    ~Camera3D();

    void tick(bool focusChangingAllowed = true);
    void subtickRotation();
    void drawFromPos(Shader* sdr, nch::Vec3f offset);
    void drawFromPos(Shader* sdr);
    void drawSkybox(Shader* sdr);
    glm::mat4 getCMatrixForOffset(nch::Vec3f offset) const;

    template<typename VecX> static VecX dirToVecX(int dir)
    {
        switch(dir) {
            case EAST:  return {-1, 0, 0};
            case WEST:  return { 1, 0, 0};
            case DOWN:  return { 0,-1, 0};
            case UP:    return { 0, 1, 0};
            case SOUTH: return { 0, 0,-1};
            case NORTH: return { 0, 0, 1};
        }
        return {0, 0, 0};
    }
    static nch::Vec3i64 dirToVecI64(int dir);
    static nch::Vec3i dirToVecI(int dir);
    static nch::Vec3f dirToVec(int dir);
    static std::string dirToString(int dir);
    static int flippedDir(int dir);
    static Vec3f getNearestAxisVec(nch::Vec3f v);
    /**
     * @brief The "natural up" a camera produces for a given orientation (no roll).
     * @param yawDeg Yaw in degrees.
     * @param pitchDeg Pitch in degrees.
     */
    static nch::Vec3f getNaturalUpFrom(float yawDeg, float pitchDeg);
    /**
     * @brief Decompose a forward direction into yaw/pitch degrees (this camera's spherical convention).
     * @param fwd Forward direction (need not be unit).
     * @return (yawDeg, pitchDeg).
     */
    static nch::Vec2f getYawPitchFrom(nch::Vec3f fwd);
    /**
     * @brief Signed roll (deg) about fwd that carries natUp onto targetUp (both projected ⟂ fwd).
     * @param fwd Forward axis (unit).
     * @param natUp The camera's natural up for its orientation.
     * @param targetUp The desired up.
     * @return 0 if either projected up is degenerate.
     */
    static float getRolledAngleAround(nch::Vec3f fwd, nch::Vec3f natUp, nch::Vec3f targetUp);
    std::string getInfo() const;
    nch::Vec3f getEstPos() const;
    nch::Vec3f getInterpolDelta() const;
    nch::Vec3f getEstInterpolPos() const;
    nch::Vec3i64 getRegPos() const;
    nch::Vec3f getSubPos() const;
    nch::Vec3i64 getIntPos() const;

    bool isFocused() const;
    bool isFreeOrientation() const;
    float getFOV() const;
    nch::Vec3f getRot() const;
    nch::Vec3f getForward() const;
    nch::Vec3f getRotYPR() const;
    nch::Vec3f getExtraRot() const;
    nch::Vec3f getEffectiveRot() const;
    float getYaw() const;
    float getPitch() const;
    float getRoll() const;
    //Yaw/pitch of the look direction expressed in the BODY's own frame (the frame where model +Y maps to
    //'up' via the minimal-arc rotation). Unlike getYaw/getPitch (which are world-frame spherical angles),
    //these stay meaningful when the body is tilted onto a wall/ceiling — use them to point a mob's head at
    //the look direction. Both equal getYaw()/getPitch() when up is +Y. Degrees; pitch is polar from +Y.
    float getLocalYaw() const;
    float getLocalPitch() const;
    float getSensitivity() const;
    std::vector<Vec4f> computeCullingPlanes() const;
    glm::mat4 getCMatrix() const;
    nch::Vec3f getUp() const;
    //The up vector the render view is actually built from (buildViewProj's 'rolledUp'): natural up rolled
    //about the effective look axis, or the pinned basis up. Unlike getUp() - which only tracks the live up
    //axis in free-orientation mode - this is correct in every mode, so anything reconstructing the render
    //view outside of drawFromPos (fullscreen sky/post passes) must use it or it will ignore camera roll.
    nch::Vec3f getRenderUp() const;
    nch::Vec3f getRight() const;
    nch::Vec3f getPerspectiveOffset() const;
    float getPerspectiveDir() const;
    //The perspective offset transformed from camera-local to world space — the displacement between the
    //eye position and the actual render camera position (what drawFromPos applies). Zero in first person.
    nch::Vec3f getPerspectiveWorldOffset() const;
    int getFacingNESW() const;

    void setFocused(bool focused);
    void setWindow(SDL_Window* win, nch::Vec2i virtualWinDims);
    void setWindow(SDL_Window* win);
    void setWindow(nch::Vec2i virtualWinDims);
    void setPos(nch::Vec3f pos);
    void setVel(nch::Vec3f vel);
    void setRot(float yaw, float pitch, float roll = 0);
    //Free-orientation mode: when enabled, subtickRotation tracks a full (forward, up) basis instead of
    //yawing/pitching about world-Y. Mouse yaw rotates forward about 'up', pitch rotates it about the
    //rolled 'right' (hard-clamped near the up axis like a classic FPS). Default off, so every other
    //camera behaves identically to before. yaw/pitch/roll are derived from the basis each subtick for
    //rendering. Players reorient 'up' via rollBy()/tumbleBy()/alignUpTo().
    void setFreeOrientation(bool v);
    //Roll the up/right basis about the current forward look axis by angRad (continuous; call each tick
    //while a roll key is held). No-op in the degenerate case where forward is parallel to up.
    void rollBy(float angRad);
    //Tumble the whole frame forward/backward by angRad: rotate both forward and up about the rolled
    //'right' axis (the look pitches over the top and the body follows), flipping you onto an adjacent
    //surface in zero-g. Continuous; call each tick while a tumble key is held.
    void tumbleBy(float angRad);
    //Slerp 'up' a fraction toward 'target' (need not be unit), carrying 'forward' by the same rotation
    //so the look direction stays put relative to the body. Used for ground-normal / gravity alignment.
    void alignUpTo(nch::Vec3f target, float frac);
    void setRotVec(nch::Vec3f rotVec);
    void setExtraRotVec(nch::Vec3f extraRotVec);
    void setFOV(float fov);
    void setNearPlane(float np);
    void setFarPlane(float fp);
    void setSensitivity(float s);
    void setOverrideMatrix(const glm::mat4& mat);
    void clearOverrideMatrix();
    //Virtual-view overrides (portal/mirror render cameras). Unlike setOverrideMatrix — a raw matrix that
    //ignores the per-mesh position offset, so it cannot express a scene camera — these participate in
    //buildViewProj, so per-chunk offset rendering (drawFromPos/getCMatrixForOffset) and
    //computeCullingPlanes all stay consistent. Both are inert until set.
    //setBasis pins the exact view basis: 'forward' replaces rotVec and 'up' replaces the
    //naturalUp/roll-derived up. setProjOverride replaces the internal glm::perspective (carrying e.g. an
    //oblique near-plane clip or a mirror X-flip), bypassing fov/nearPlane/farPlane while set.
    void setBasis(nch::Vec3f forward, nch::Vec3f up);
    void clearBasis();
    void setProjOverride(const glm::mat4& proj);
    void clearProjOverride();
    void setPerspective(nch::Vec3f perspectiveOffset, float perspectiveDir);
private:
    /**
     * @brief Helper function to update 'cMatrix', a perspective matrix for this 'Camera3D'.
     * @brief See 'fov', 'nearPlane', and 'farPlane' for more info.
     * @param pos The current position of the camera (Camera3D::pos is not accurate in between ticks - see draw() for more info)
     */
    void updateCamMatrix(nch::Vec3f pos);
    //Shared view*proj construction for updateCamMatrix/getCMatrixForOffset, so the two can't drift apart.
    //'effRot' must already include the perspectiveDir flip where applicable.
    glm::mat4 buildViewProj(nch::Vec3f pos, nch::Vec3f effRot) const;
    void updateRegAndSubPos() const;
    glm::vec3 computeNaturalUp() const;
    glm::vec3 computeRolledUp(glm::vec3 naturalUp, glm::vec3 forward) const;
    //Free-orientation helpers (see setFreeOrientation). subtickFreeOrientation integrates the mouse
    //delta into the (forward, up) basis and writes back the derived yaw/pitch/roll + up.
    void subtickFreeOrientation(float dmx, float dmy);
    void commitBasis(glm::vec3 forward, glm::vec3 upv);
    //The forward look vector rotated into the body frame (undoing the minimal +Y->up rotation). Basis for
    //getLocalYaw/getLocalPitch.
    nch::Vec3f getForwardInBodyFrame() const;
    static glm::vec3 rotateAbout(glm::vec3 v, glm::vec3 axis, float angRad);

    bool focused = false;
    bool freeOrient = false; //See setFreeOrientation. When true, 'up' is the live up axis and may be non-+Y.
    uint64_t ticksSinceLastUnfocus = 0;
    uint64_t lastTickTimeNS = 0;
    uint64_t tickTimeNS = 0;

    SDL_Window* sdlWin = nullptr; int sdlWinW = 1; int sdlWinH = 1;
    float fov = 65.0f;          //[F]ield [o]f [v]iew, in degrees.    
    float nearPlane = 0.1f;     //How far away "near-clipping" should take place (anything behind this should be invisible)
    float farPlane = 1000.0f;   //How far away "far-clipping" should take place (anything beyond this should be invisible)
    Vec3f up = {0.0f, 1.0f, 0.0f};
    glm::mat4 cMatrix;
    bool useOverrideMatrix = false;
    glm::mat4 overrideMatrix = glm::mat4(1.0f);
    bool useBasisOverride = false;
    Vec3f basisUp = {0.0f, 1.0f, 0.0f}; //See setBasis
    bool useProjOverride = false;
    glm::mat4 projOverride = glm::mat4(1.0f);

    mutable Vec3i64 regPos = 0; //Region pos
    mutable Vec3f subPos = 0;   //Sub pos (in [0, 31.9999])
    mutable Vec3f lPos = 0;     //Logical pos (truncated to subpos every tick)

    Vec3f rotVec = {0.0f, 0.0f, -1.0f};
    Vec3f extraRotVec = {0.0f, 0.0f, 0.0f};
    float yaw = 270;  //Nodding no
    float pitch = 90; //Nodding yes
    float roll = 0;   //Tilting head left/right
    Vec3f vel = 0;
    float sensitivity = 0.15f;
    Vec3f perspectiveOffset = {0, 0, 0};
    float perspectiveDir = 1.0f;

    static const std::vector<std::string> dirStrings;
}; }
