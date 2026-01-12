#pragma once
#include <GL/glew.h>
#include <SDL2/SDL.h>
#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <string>
#include <vector>

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

    Camera3D(SDL_Window* win);
    ~Camera3D();

    void tick();
    void subtickRotation();
    void drawFromPos(GLuint shaderID, glm::vec3 offset);

    static glm::vec3 dirToVec(int dir);
    static std::string dirToString(int dir);
    static int flippedDir(int dir);
    std::string getInfo();
    glm::vec3 getEstPos(); glm::i64vec3 getRegPos(); glm::vec3 getSubPos();

    glm::vec3 getRot() const;
    float getYaw() const;
    float getPitch() const;
    std::vector<glm::vec4> getFrustumPlanes() const;
    glm::vec3 getUp() const;
    int getFacingNESW() const;

    void setPos(glm::vec3 pos);
    void setVel(glm::vec3 vel);
    void setFOV(float fov);
    void setNearPlane(float np);
    void setFarPlane(float fp);
private:
    /// @brief Helper function to update 'cMatrix', a perspective matrix for this 'Camera3D' that takes in a number of parameters.
    /// @brief See 'fov', 'nearPlane', and 'farPlane' for more info.
    /// @param pos The current position of the camera (Camera3D::pos is not accurate in between ticks - see draw() for more info)
    void updateCamMatrix(glm::vec3 pos);
    void updateRegAndSubPos();

    bool focused = false;
    uint64_t lastTickTimeNS = 0;
    uint64_t tickTimeNS = 0;

    SDL_Window* sdlWin = nullptr; int sdlWinW = 1; int sdlWinH = 1;
    float fov = 65.0f;          //[F]ield [o]f [v]iew, in degrees.    
    float nearPlane = 0.1f;     //How far away "near-clipping" should take place (anything behind this should be invisible)
    float farPlane = 1000.0f;   //How far away "far-clipping" should take place (anything beyond this should be invisible)
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 cMatrix;
    std::vector<glm::vec4> frustumPlanes;

    glm::i64vec3 regPos = glm::i64vec3(0);  //Region pos
    glm::vec3 subPos = glm::vec3(0);        //Sub pos (in [0, 31.9999])
    glm::vec3 lPos = glm::vec3(0);          //Logical pos (truncated to subpos every tick)

    glm::vec3 rot = glm::vec3(0.0f, 0.0f, -1.0f);
    float yaw = 270;  //Nodding no
    float pitch = 90; //Nodding yes
    glm::vec3 vel = glm::vec3(0);

    static const std::vector<std::string> dirStrings;
}; }
