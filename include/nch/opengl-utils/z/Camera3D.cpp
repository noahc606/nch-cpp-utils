#include"Camera3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/main-loop-driver.h>
#include <cmath>
#include <sstream>
using namespace nch;

constexpr double NCH_PI = 3.14159265358979323846;
const std::vector<std::string> Camera3D::dirStrings = {
    "east", "west", "down", "up", "south", "north", "unknown",
};

Camera3D::Camera3D() {}
Camera3D::Camera3D(SDL_Window* win) : Camera3D() {
    setWindow(win);
}
Camera3D::~Camera3D(){}

void Camera3D::tick(bool focusChangingAllowed)
{
    ticksSinceLastUnfocus++;
    lastTickTimeNS = Timer::getCurrentTimeNS();
    if(sdlWinW<=0 || sdlWinH<=0) {
        throw std::runtime_error(Log::getFormattedString(
            "Window has invalid dimensions %dx%d - did you forget to call Camera3D::setWindow(SDL_Window*) and Camera3D::setWindow(nch::Vec2i)?", sdlWinW, sdlWinH
        ));
        return;
    }

    /* Camera focus */
    {
        //Update logical, regional, and sub position every tick
        updateRegAndSubPos();

        //Toggle whether control-focused using ESCAPE
        if(focusChangingAllowed && Input::keyDownTime(SDLK_ESCAPE)==1) {
            setFocused(!focused);
        }
    }

}
void Camera3D::drawFromPos(Shader* sdr, Vec3f offset)
{
    //Transform perspectiveOffset from camera-local to world space and add to offset
    glm::vec3 fwd   = glm::vec3(rotVec);
    glm::vec3 natUp = computeNaturalUp();
    glm::vec3 right = glm::normalize(glm::cross(fwd, natUp));
    glm::vec3 worldOff = right*perspectiveOffset.x + natUp*perspectiveOffset.y + fwd*perspectiveOffset.z;
    offset = offset + Vec3f(worldOff.x, worldOff.y, worldOff.z);

    //Find interpolated position ('ipos') between two ticks:
    //Uses current position, tick progress, and current velocity.
    uint64_t currTimeNS = Timer::getCurrentTimeNS();
    double tickProgress = (double)(currTimeNS-lastTickTimeNS)/MainLoopDriver::getTargetNSPT();
    Vec3f iLPos = lPos+vel*(float)tickProgress+offset;

    Vec3f savedRotVec = rotVec;
    rotVec = rotVec * perspectiveDir;
    updateCamMatrix(iLPos);
    rotVec = savedRotVec;

    const std::string& uniCamPos = sdr->getUniformCamPos();
    const std::string& uniCamMat = sdr->getUniformCamMatrix();
    if(uniCamPos!="") {
        glUniform3f(sdr->getUniformLoc(uniCamPos), iLPos.x, iLPos.y, iLPos.z);
    }
    if(uniCamMat!="") {
        glUniformMatrix4fv(sdr->getUniformLoc(uniCamMat), 1, GL_FALSE, glm::value_ptr(cMatrix));
    }
}
void Camera3D::drawFromPos(Shader* sdr) {
    drawFromPos(sdr, {0, 0, 0});
}
void Camera3D::setPerspective(Vec3f perspectiveOffset, float perspectiveDir) {
    Camera3D::perspectiveOffset = perspectiveOffset;
    Camera3D::perspectiveDir = perspectiveDir;
}
glm::mat4 Camera3D::getCMatrixForOffset(Vec3f offset) const
{
    uint64_t currTimeNS = Timer::getCurrentTimeNS();
    double tickProgress = (double)(currTimeNS-lastTickTimeNS)/MainLoopDriver::getTargetNSPT();
    Vec3f pos = lPos+vel*(float)tickProgress+offset;
    glm::vec3 forward = glm::vec3(rotVec);
    glm::vec3 rolledUp = computeRolledUp(computeNaturalUp(), forward);
    glm::mat4 view = glm::lookAt((glm::vec3)pos, (glm::vec3)(pos+rotVec), rolledUp);
    glm::mat4 proj = glm::perspective(glm::radians(fov), ((float)sdlWinW/(float)sdlWinH), nearPlane, farPlane);
    return proj*view;
}
void Camera3D::drawSkybox(Shader* sdr)
{
    //For skybox rendering, put camera @ origin while keeping rotation
    updateCamMatrix({0, 0, 0});
    const std::string& uniCamPos = sdr->getUniformCamPos();
    const std::string& uniCamMat = sdr->getUniformCamMatrix();
    if(uniCamPos!="") {
        glUniform3f(sdr->getUniformLoc(uniCamPos), 0.0f, 0.0f, 0.0f);
    }
    if(uniCamMat!="") {
        glUniformMatrix4fv(sdr->getUniformLoc(uniCamMat), 1, GL_FALSE, glm::value_ptr(cMatrix));
    }
}

Vec3i64 Camera3D::dirToVecI64(int dir) {
    return dirToVecX<Vec3i64>(dir);
}
Vec3i Camera3D::dirToVecI(int dir) {
    return dirToVecX<Vec3i>(dir);
}
Vec3f Camera3D::dirToVec(int dir) {
    return dirToVecX<Vec3f>(dir);
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
    ss << "ypr: [" << yaw << ", " << pitch << ", " << roll << "], ";
    ss << "facing: " << Camera3D::dirToString(getFacingNESW()) << "}";
    return ss.str();
}

Vec3f Camera3D::getEstPos() {
    updateRegAndSubPos();
    Vec3f ret = Vec3f(regPos.x*32, regPos.y*32, regPos.z*32)+subPos;
    return ret;
}
Vec3f Camera3D::getInterpolDelta() const {
    //Interpolated local offset between two ticks
    //Uses local position, tick progress, and current velocity.
    //Does not include regPos, avoiding large-coordinate precision issues.
    uint64_t currTimeNS = Timer::getCurrentTimeNS();
    double tickProgress = (double)(currTimeNS-lastTickTimeNS)/MainLoopDriver::getTargetNSPT();
    return vel*(float)tickProgress;
}
Vec3f Camera3D::getEstInterpolPos() {
    return lPos+getInterpolDelta()+regPos.toFloat()*32;
}
Vec3i64 Camera3D::getRegPos() {
    updateRegAndSubPos();
    return regPos;
}
Vec3f Camera3D::getSubPos() {
    updateRegAndSubPos();
    return subPos;
}
Vec3i64 Camera3D::getIntPos() {
    updateRegAndSubPos();
    return regPos*32+Vec3i64(std::floor(subPos.x), std::floor(subPos.y), std::floor(subPos.z));
}

bool Camera3D::isFocused() const {
    return focused;
}
float Camera3D::getFOV() const {
    return fov;
}
Vec3f Camera3D::getRot() const {
    return rotVec;
}
float Camera3D::getYaw() const {
    return yaw;
}
float Camera3D::getPitch() const {
    return pitch;
}
float Camera3D::getRoll() const {
    return roll;
}
float Camera3D::getSensitivity() const {
    return sensitivity;
}
glm::mat4 Camera3D::getCMatrix() const {
    return cMatrix;
}
std::vector<glm::vec4> Camera3D::computeCullingPlanes() const {
    glm::mat4 mvp = getCMatrixForOffset({0, 0, 0});
    std::vector<glm::vec4> planes(6);
    planes[0] = {mvp[0][3]+mvp[0][0], mvp[1][3]+mvp[1][0], mvp[2][3]+mvp[2][0], mvp[3][3]+mvp[3][0]};
    planes[1] = {mvp[0][3]-mvp[0][0], mvp[1][3]-mvp[1][0], mvp[2][3]-mvp[2][0], mvp[3][3]-mvp[3][0]};
    planes[2] = {mvp[0][3]+mvp[0][1], mvp[1][3]+mvp[1][1], mvp[2][3]+mvp[2][1], mvp[3][3]+mvp[3][1]};
    planes[3] = {mvp[0][3]-mvp[0][1], mvp[1][3]-mvp[1][1], mvp[2][3]-mvp[2][1], mvp[3][3]-mvp[3][1]};
    planes[4] = {mvp[0][3]+mvp[0][2], mvp[1][3]+mvp[1][2], mvp[2][3]+mvp[2][2], mvp[3][3]+mvp[3][2]};
    planes[5] = {mvp[0][3]-mvp[0][2], mvp[1][3]-mvp[1][2], mvp[2][3]-mvp[2][2], mvp[3][3]-mvp[3][2]};
    for(auto& plane : planes) {
        plane /= glm::length(glm::vec3(plane));
    }
    return planes;
}
Vec3f Camera3D::getUp() const {
    return up;
}
Vec3f Camera3D::getPerspectiveOffset() const {
    return perspectiveOffset;
}
float Camera3D::getPerspectiveDir() const {
    return perspectiveDir;
}

int Camera3D::getFacingNESW() const
{
    if(yaw<=45) return WEST;
    if(yaw<=135) return NORTH;
    if(yaw<=225) return EAST;
    if(yaw<=315) return SOUTH;
    return WEST;
}

void Camera3D::setFocused(bool focused) {
    if(!sdlWin) throw std::runtime_error("Cameras with a null SDL window are valid, but cannot be focused");
    if(Camera3D::focused!=focused) {
        SDL_WarpMouseInWindow(sdlWin, sdlWinW/2, sdlWinH/2);
    }
    Camera3D::focused = focused;

    if(!focused) {
        ticksSinceLastUnfocus = 0;
    }
}
void Camera3D::setWindow(SDL_Window* win, nch::Vec2i virtualWinDims) {
    setWindow(win);
    setWindow(virtualWinDims);
}
void Camera3D::setWindow(SDL_Window* win) {
    sdlWin = win;
}
void Camera3D::setWindow(Vec2i virtualWinDims) {
    if(virtualWinDims.x<=0 || virtualWinDims.y<=0) {
        throw std::invalid_argument("Both dimensions must be positive");
    }
    sdlWinW = virtualWinDims.x;
    sdlWinH = virtualWinDims.y;
}

void Camera3D::setPos(Vec3f pos) {
    Camera3D::regPos = Vec3i64(0, 0, 0);
    Camera3D::lPos = pos;
    updateRegAndSubPos();
}
void Camera3D::setVel(Vec3f vel) {
    Camera3D::vel = vel;
}
void Camera3D::setRot(float yaw, float pitch, float roll) {
    yaw = fmod(fmod(yaw, 360.f) + 360.f, 360.f);
    if(pitch>180.f) pitch = 180.f;
    if(pitch<0.f) pitch = 0.f;
    roll = fmod(fmod(roll, 360.f) + 360.f, 360.f);

	//Update 'rot' based on current yaw and pitch
	float the = pitch*NCH_PI/180.; //From pitch
	float phi = yaw*NCH_PI/180.;   //From yaw

    setRotVec({
		std::sin(the)*std::cos(phi),
		std::cos(the),
		std::sin(the)*std::sin(phi)
    });
    Camera3D::yaw = yaw;
    Camera3D::pitch = pitch;
    Camera3D::roll = roll;
}
void Camera3D::setRotVec(Vec3f rotVec) {
    Camera3D::rotVec = rotVec;
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
void Camera3D::setSensitivity(float s) {
    Camera3D::sensitivity = s;
}
void Camera3D::setOverrideMatrix(const glm::mat4& mat) {
    overrideMatrix = mat;
    useOverrideMatrix = true;
}
void Camera3D::clearOverrideMatrix() {
    useOverrideMatrix = false;
}

void Camera3D::updateCamMatrix(Vec3f pos)
{
    if(useOverrideMatrix) { cMatrix = overrideMatrix; return; }
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    //Calculate 'rolledUp' from spherical-coordinate-derived natural up
    glm::vec3 forward = glm::vec3(rotVec);
    glm::vec3 rolledUp = computeRolledUp(computeNaturalUp(), forward);
    //Calculate 'view' from lookAt and 'proj'
    view = glm::lookAt((glm::vec3)pos, (glm::vec3)(pos+rotVec), rolledUp);
    proj = glm::perspective(glm::radians(fov), ((float)sdlWinW/(float)sdlWinH), nearPlane, farPlane);
    //Calculate 'cMatrix' from 'view' and 'proj'
    cMatrix = proj*view;
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
    if(!sdlWin) throw std::runtime_error("Cameras with a null SDL window are valid, but cannot use subtickRotation()");
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
        if(ticksSinceLastUnfocus<3) {
            dmx = 0;
            dmy = 0;
        }
        yaw -= dmx * sensitivity;
        pitch -= dmy * sensitivity;
    }

    setRot(yaw, pitch, roll);
}
glm::vec3 Camera3D::computeNaturalUp() const
{
    //Derived from spherical coordinate partial derivative w.r.t. pitch.
    //Always perpendicular to rotVec, so lookAt never degenerates at the poles.
    float pitchRad = pitch*(float)NCH_PI/180.0f;
    float yawRad = yaw*(float)NCH_PI/180.0f;
    return glm::vec3(
        -std::cos(pitchRad)*std::cos(yawRad),
         std::sin(pitchRad),
        -std::cos(pitchRad)*std::sin(yawRad)
    );
}
glm::vec3 Camera3D::computeRolledUp(glm::vec3 naturalUp, glm::vec3 forward) const
{
    float rollRad = roll*(float)NCH_PI/180.0f;
    return glm::rotate(glm::mat4(1.0f), rollRad, forward)*glm::vec4(naturalUp, 0.0f);
}
