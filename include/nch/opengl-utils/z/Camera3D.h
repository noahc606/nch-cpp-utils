#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "nch/math-utils/vec2.h"
#include "nch/math-utils/vec3.h"

namespace nch { class Camera3D {
public:
    enum Direction {
        EAST,  // -X
        WEST,  // +X
        DOWN,  // -Y
        UP,    // +Y
        SOUTH, // -Z
        NORTH, // +Z
        UNKNOWN,
    };

    Camera3D();
    Camera3D(SDL_Window* win);
    ~Camera3D();

    void tick();
    void subtickRotation();
    void drawFromPos(GLuint shaderID, nch::Vec3f offset);
    void drawFromPos(GLuint shaderID);
    void drawSkybox(GLuint shaderID);

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
    std::string getInfo();
    nch::Vec3f getEstPos();
    nch::Vec3i64 getRegPos();
    nch::Vec3f getSubPos();
    nch::Vec3i64 getIntPos();

    nch::Vec3f getRot() const;
    float getYaw() const;
    float getPitch() const;
    float getRoll() const;
    std::vector<glm::vec4> getFrustumPlanes() const;
    nch::Vec3f getUp() const;
    int getFacingNESW() const;

    void setWindow(SDL_Window* win);
    void setWindow(nch::Vec2i virtualWinDims);
    void setPos(nch::Vec3f pos);
    void setVel(nch::Vec3f vel);
    void setRot(float yaw, float pitch, float roll = 0);
    void setFOV(float fov);
    void setNearPlane(float np);
    void setFarPlane(float fp);
private:
    /// @brief Helper function to update 'cMatrix', a perspective matrix for this 'Camera3D' that takes in a number of parameters.
    /// @brief See 'fov', 'nearPlane', and 'farPlane' for more info.
    /// @param pos The current position of the camera (Camera3D::pos is not accurate in between ticks - see draw() for more info)
    void updateCamMatrix(nch::Vec3f pos);
    void updateRegAndSubPos();

    bool focused = false;
    uint64_t lastTickTimeNS = 0;
    uint64_t tickTimeNS = 0;

    SDL_Window* sdlWin = nullptr; int sdlWinW = 1; int sdlWinH = 1;
    float fov = 65.0f;          //[F]ield [o]f [v]iew, in degrees.    
    float nearPlane = 0.1f;     //How far away "near-clipping" should take place (anything behind this should be invisible)
    float farPlane = 1000.0f;   //How far away "far-clipping" should take place (anything beyond this should be invisible)
    Vec3f up = {0.0f, 1.0f, 0.0f};
    glm::mat4 cMatrix;
    std::vector<glm::vec4> frustumPlanes;

    Vec3i64 regPos = 0; //Region pos
    Vec3f subPos = 0;   //Sub pos (in [0, 31.9999])
    Vec3f lPos = 0;     //Logical pos (truncated to subpos every tick)

    Vec3f rot = {0.0f, 0.0f, -1.0f};
    float yaw = 270;  //Nodding no
    float pitch = 90; //Nodding yes
    float roll = 0;   //Tilting head left/right
    Vec3f vel = 0;

    static const std::vector<std::string> dirStrings;
}; }
