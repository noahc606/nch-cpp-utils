#include"Camera3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nch/cpp-utils/timer.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/main-loop-driver.h>
#include <sstream>

using namespace nch;

const std::vector<std::string> Camera3D::dirStrings = {
    "east", "west", "down", "up", "south", "north", "unknown",
};

Camera3D::Camera3D(SDL_Window* win) {
    for(int i = 0; i<6; i++)
        frustumPlanes.push_back({0, 0, 0, 0});
    
    sdlWin = win;
}
Camera3D::~Camera3D(){}

void Camera3D::tick()
{
    lastTickTimeNS = Timer::getCurrentTimeNS();
    if(sdlWin==nullptr) return;
    SDL_GetWindowSize(sdlWin, &sdlWinW, &sdlWinH);

    /* Camera focus */
    {
        //Update logical, regional, and sub position every tick
        updateRegAndSubPos();

        //Toggle whether control-focused using ESCAPE
        if(Input::keyDownTime(SDLK_ESCAPE)==1) {
            focused = !focused;
            if(focused) {
                SDL_WarpMouseInWindow(sdlWin, sdlWinW/2, sdlWinH/2);
            }
        }
    }

}
void Camera3D::drawFromPos(GLuint shaderID, glm::vec3 offset)
{
    //Find interpolated position ('ipos') between two ticks:
    //Uses current position, tick progress, and current velocity.
    uint64_t currTimeNS = Timer::getCurrentTimeNS();
    double tickProgress = (double)(currTimeNS-lastTickTimeNS)/MainLoopDriver::getTargetNSPT();
    glm::vec3 iLPos = lPos+vel*(float)tickProgress+offset;

    updateCamMatrix(iLPos);
    glUniform3f(glGetUniformLocation(shaderID, "camPos"), iLPos.x, iLPos.y, iLPos.z);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "camMatrix"), 1, GL_FALSE, glm::value_ptr(cMatrix));
}

glm::vec3 Camera3D::dirToVec(int dir)
{
    switch(dir) {
        case EAST:  return {-1,00,00};
        case WEST:  return {01,00,00};
        case DOWN:  return {00,-1,00};
        case UP:    return {00,01,00};
        case SOUTH: return {00,00,-1};
        case NORTH: return {00,00,01};
    }
    return {0,0,0};
}
std::string Camera3D::dirToString(int dir)
{
    if(dir>=EAST && dir<=NORTH) return dirStrings[dir];
    return dirStrings[UNKNOWN];
}
int Camera3D::flippedDir(int dir)
{
    switch(dir) {
        case EAST:  return WEST;
        case WEST:  return EAST;
        case DOWN:  return UP;
        case UP:    return DOWN;
        case SOUTH: return NORTH;
        case NORTH: return SOUTH;
    }

    return UNKNOWN;
}

std::string Camera3D::getInfo() {
    auto epos = getEstPos();

    std::stringstream ss;
    ss << "Camera={xyz: [" << epos.x << ", " << epos.y << ", " << epos.z << "], ";
    ss << "yp: [" << yaw << ", " << pitch << "], ";
    ss << "facing: " << Camera3D::dirToString(getFacingNESW()) << "}";
    return ss.str();
}

glm::vec3 Camera3D::getEstPos() {
    updateRegAndSubPos();
    glm::vec3 ret = glm::vec3(regPos.x*32, regPos.y*32, regPos.z*32)+subPos;
    return ret;
}
glm::i64vec3 Camera3D::getRegPos() {
    updateRegAndSubPos();
    return regPos;
}
glm::vec3 Camera3D::getSubPos() {
    updateRegAndSubPos();
    return subPos;
}

glm::vec3 Camera3D::getRot() const {
    return rot;
}
float Camera3D::getYaw() const {
    return yaw;
}
float Camera3D::getPitch() const {
    return pitch;
}
std::vector<glm::vec4> Camera3D::getFrustumPlanes() const {
    return frustumPlanes;
}
glm::vec3 Camera3D::getUp() const {
    return up;
}

int Camera3D::getFacingNESW() const
{
    if(yaw<=45) return WEST;
    if(yaw<=135) return NORTH;
    if(yaw<=225) return EAST;
    if(yaw<=315) return SOUTH;
    return WEST;
}

void Camera3D::setPos(glm::vec3 pos) {
    Camera3D::regPos = glm::i64vec3(0, 0, 0);
    Camera3D::lPos = pos;
    updateRegAndSubPos();
}
void Camera3D::setVel(glm::vec3 vel) {
    Camera3D::vel = vel;
}

void Camera3D::setFOV(float fov) {
    Camera3D::fov = fov;
}
void Camera3D::setNearPlane(float np) {
    Camera3D::nearPlane = np;
}
void Camera3D::setFarPlane(float fp) {
    Camera3D::farPlane = fp;
}

void Camera3D::updateCamMatrix(glm::vec3 pos)
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    view = glm::lookAt(pos, pos+rot, up);
    proj = glm::perspective(glm::radians(fov), ((float)sdlWinW/(float)sdlWinH), nearPlane, farPlane);

    cMatrix = proj*view;

    //Calculate frustum planes for the camera.
    frustumPlanes[0] = glm::vec4(cMatrix[0][3]+cMatrix[0][0], cMatrix[1][3]+cMatrix[1][0], cMatrix[2][3]+cMatrix[2][0], cMatrix[3][3]+cMatrix[3][0]); // Left
    frustumPlanes[1] = glm::vec4(cMatrix[0][3]-cMatrix[0][0], cMatrix[1][3]-cMatrix[1][0], cMatrix[2][3]-cMatrix[2][0], cMatrix[3][3]-cMatrix[3][0]); // Right
    frustumPlanes[2] = glm::vec4(cMatrix[0][3]+cMatrix[0][1], cMatrix[1][3]+cMatrix[1][1], cMatrix[2][3]+cMatrix[2][1], cMatrix[3][3]+cMatrix[3][1]); // Bottom
    frustumPlanes[3] = glm::vec4(cMatrix[0][3]-cMatrix[0][1], cMatrix[1][3]-cMatrix[1][1], cMatrix[2][3]-cMatrix[2][1], cMatrix[3][3]-cMatrix[3][1]); // Top
    frustumPlanes[4] = glm::vec4(cMatrix[0][3]+cMatrix[0][2], cMatrix[1][3]+cMatrix[1][2], cMatrix[2][3]+cMatrix[2][2], cMatrix[3][3]+cMatrix[3][2]); // Near
    frustumPlanes[5] = glm::vec4(cMatrix[0][3]-cMatrix[0][2], cMatrix[1][3]-cMatrix[1][2], cMatrix[2][3]-cMatrix[2][2], cMatrix[3][3]-cMatrix[3][2]); // Far

    //Normalize the planes
    //A 'plane'==vec4(A,B,C,D) is defined by Ax+By+Cz=D
    for(auto& plane : frustumPlanes) {
        float length = glm::length(glm::vec3(plane.x, plane.y, plane.z));
        plane /= length;
    }
}
void Camera3D::updateRegAndSubPos()
{
    //Add to 'regPos' from 'lPos'
    regPos.x += std::floor(lPos.x/32.0);
    regPos.y += std::floor(lPos.y/32.0);
    regPos.z += std::floor(lPos.z/32.0);

    //Truncate 'lPos'
    lPos.x -= 32*std::floor(lPos.x/32.0);
    lPos.y -= 32*std::floor(lPos.y/32.0);
    lPos.z -= 32*std::floor(lPos.z/32.0);

    //Get 'subPos' from truncated 'lPos'
    subPos = lPos;
}
void Camera3D::subtickRotation()
{
    if(focused) {
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_ShowCursor(SDL_ENABLE);
        return;
    }

    //Update camera direction based on mouse movements
    int mx = Input::getMouseX();
    int my = Input::getMouseY();
    if(mx!=sdlWinW/2 || my!=sdlWinH/2) {
        int lmx = mx;
        int lmy = my;
        SDL_WarpMouseInWindow(sdlWin, sdlWinW/2, sdlWinH/2);
        int dmx = sdlWinW/2-lmx;
        int dmy = sdlWinH/2-lmy;

        yaw -= dmx/4.;   //Yaw
        pitch -= dmy/4.;   //Pitch

        while(yaw<0) yaw += 360;
        while(yaw>360) yaw -= 360;
        if(pitch>179.9) pitch = 179.9;
        if(pitch<0.01) pitch = 0.01;
    }

	//Update 'rot' based on current yaw and pitch
	float the = pitch*M_PI/180.;   //From pitch
	float phi = yaw*M_PI/180.;   //From yaw
	rot = glm::vec3(
		std::sin(the)*std::cos(phi),
		std::cos(the),
		std::sin(the)*std::sin(phi)
	);
}
