#include"Camera3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include <nch/math-utils/consts.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/main-loop-driver.h>
#include <cmath>
#include <sstream>
using namespace nch;

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
    offset = offset + getPerspectiveWorldOffset();

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
    //Must match the render matrix drawFromPos produces (perspective offset + dir flip included), or
    //anything derived from this — frustum culling planes, world->screen projection — is wrong whenever
    //a third-person perspective is active.
    uint64_t currTimeNS = Timer::getCurrentTimeNS();
    double tickProgress = (double)(currTimeNS-lastTickTimeNS)/MainLoopDriver::getTargetNSPT();
    Vec3f pos = lPos+vel*(float)tickProgress+offset+getPerspectiveWorldOffset();
    Vec3f effRot = (rotVec*perspectiveDir+extraRotVec).normalized();
    return buildViewProj(pos, effRot);
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
    if(dir>=_WEST && dir<=NORTH) return dirStrings[dir];
    return dirStrings[UNKNOWN];
}
int Camera3D::flippedDir(int dir)
{
    switch(dir) {
        case _EAST:  return _WEST;
        case _WEST:  return _EAST;
        case DOWN:  return UP;
        case UP:    return DOWN;
        case SOUTH: return NORTH;
        case NORTH: return SOUTH;
    }

    return UNKNOWN;
}

std::string Camera3D::getInfo() const {
    auto epos = getEstPos();

    std::stringstream ss;
    ss << "Camera={xyz: [" << epos.x << ", " << epos.y << ", " << epos.z << "], ";
    ss << "ypr: [" << yaw << ", " << pitch << ", " << roll << "], ";
    ss << "facing: " << Camera3D::dirToString(getFacingNESW()) << "}";
    return ss.str();
}

Vec3f Camera3D::getEstPos() const {
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
Vec3f Camera3D::getEstInterpolPos() const {
    return lPos+getInterpolDelta()+regPos.toFloat()*32;
}
Vec3i64 Camera3D::getRegPos() const {
    updateRegAndSubPos();
    return regPos;
}
Vec3f Camera3D::getSubPos() const {
    updateRegAndSubPos();
    return subPos;
}
Vec3i64 Camera3D::getIntPos() const {
    updateRegAndSubPos();
    return regPos*32+Vec3i64(std::floor(subPos.x), std::floor(subPos.y), std::floor(subPos.z));
}

bool Camera3D::isFocused() const { return focused; }
bool Camera3D::isFreeOrientation() const { return freeOrient; }
float Camera3D::getFOV() const { return fov; }
Vec3f Camera3D::getRot() const { return rotVec; }
Vec3f Camera3D::getForward() const { return rotVec.normalized(); }
Vec3f Camera3D::getRotYPR() const { return Vec3f(yaw, pitch, roll); }
Vec3f Camera3D::getExtraRot() const { return extraRotVec; }
Vec3f Camera3D::getEffectiveRot() const { return (rotVec+extraRotVec).normalized(); }
float Camera3D::getYaw() const { return yaw; }
float Camera3D::getPitch() const { return pitch; }
float Camera3D::getRoll() const { return roll; }
float Camera3D::getLocalYaw() const {
    glm::vec3 fl = forwardInBodyFrame();
    return std::atan2(fl.z, fl.x) * (float)nch::deg_rad;
}
float Camera3D::getLocalPitch() const {
    glm::vec3 fl = forwardInBodyFrame();
    return std::acos(glm::clamp(fl.y, -1.0f, 1.0f)) * (float)nch::deg_rad;
}
float Camera3D::getSensitivity() const { return sensitivity; }
glm::mat4 Camera3D::getCMatrix() const { return cMatrix; }
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
Vec3f Camera3D::getRight() const {
    Vec3f r = rotVec.cross(up);
    return r.normalized();
}
Vec3f Camera3D::getPerspectiveOffset() const {
    return perspectiveOffset;
}
float Camera3D::getPerspectiveDir() const {
    return perspectiveDir;
}
Vec3f Camera3D::getPerspectiveWorldOffset() const {
    //Basis from the UNFLIPPED look direction: perspectiveDir only flips where the camera faces,
    //not where it sits (THIRD_PERSON_ALT places the camera forward, then looks back).
    glm::vec3 fwd   = glm::vec3(getEffectiveRot());
    glm::vec3 natUp = computeNaturalUp();
    glm::vec3 right = glm::normalize(glm::cross(fwd, natUp));
    glm::vec3 worldOff = right*perspectiveOffset.x + natUp*perspectiveOffset.y + fwd*perspectiveOffset.z;
    return Vec3f(worldOff.x, worldOff.y, worldOff.z);
}

int Camera3D::getFacingNESW() const
{
    if(yaw<=45) return _EAST;
    if(yaw<=135) return NORTH;
    if(yaw<=225) return _WEST;
    if(yaw<=315) return SOUTH;
    return _EAST;
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
	float the = pitch*nch::rad_deg; //From pitch
	float phi = yaw*nch::rad_deg;   //From yaw

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
void Camera3D::setFreeOrientation(bool v) {
    if(v && !freeOrient) {
        //Seed the live 'up' from the current orientation so enabling the mode doesn't jump the view.
        glm::vec3 f = glm::normalize(glm::vec3(getEffectiveRot()));
        glm::vec3 u = computeRolledUp(computeNaturalUp(), f);
        up = Vec3f(u.x, u.y, u.z);
    }
    freeOrient = v;
}
void Camera3D::rollBy(float angRad) {
    if(angRad==0.0f) return;
    glm::vec3 f = glm::normalize(glm::vec3(rotVec));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    //Roll banks the whole body about its FACING axis (forward projected onto the plane perpendicular to up),
    //not the raw look vector. Using the body axis makes roll a consistent left/right bank in every
    //orientation and independent of how far up/down you're currently looking. +angRad banks toward body-right.
    glm::vec3 bodyFwd = f - u*glm::dot(f, u);
    if(glm::length(bodyFwd)<1e-4f) return; //looking straight along body up/down: bank axis is undefined
    bodyFwd = glm::normalize(bodyFwd);
    glm::vec3 nf = glm::normalize(rotateAbout(f, bodyFwd, angRad));
    glm::vec3 nu = glm::normalize(rotateAbout(u, bodyFwd, angRad));
    commitBasis(nf, nu);
}
void Camera3D::tumbleBy(float angRad) {
    if(angRad==0.0f) return;
    glm::vec3 f = glm::normalize(glm::vec3(rotVec));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    //Tumble somersaults the whole body forward/backward about its SIDE axis (body-right), again derived from
    //the facing rather than the look, so it's a consistent forward/back flip regardless of look pitch.
    //+angRad tips the body forward (the top of the head rotates toward the facing direction).
    glm::vec3 bodyFwd = f - u*glm::dot(f, u);
    if(glm::length(bodyFwd)<1e-4f) return; //looking straight along body up/down: facing is undefined
    bodyFwd = glm::normalize(bodyFwd);
    glm::vec3 bodyRight = glm::normalize(glm::cross(bodyFwd, u));
    glm::vec3 nf = glm::normalize(rotateAbout(f, bodyRight, angRad));
    glm::vec3 nu = glm::normalize(rotateAbout(u, bodyRight, angRad));
    commitBasis(nf, nu);
}
void Camera3D::alignUpTo(Vec3f target, float frac) {
    glm::vec3 t = glm::vec3(target.x, target.y, target.z);
    if(glm::length(t)<1e-6f) return;
    t = glm::normalize(t);
    glm::vec3 f = glm::normalize(glm::vec3(rotVec));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    float d = glm::clamp(glm::dot(u, t), -1.0f, 1.0f);
    float ang = std::acos(d);
    if(ang<1e-4f) return; //already aligned
    glm::vec3 axis = glm::cross(u, t);
    if(glm::length(axis)<1e-6f) {
        //180° flip: pick any axis perpendicular to u (and to f if possible).
        axis = glm::cross(u, f);
        if(glm::length(axis)<1e-6f) axis = (std::abs(u.x)<0.9f) ? glm::vec3(1,0,0) : glm::vec3(0,0,1);
    }
    axis = glm::normalize(axis);
    float rot = ang*glm::clamp(frac, 0.0f, 1.0f);
    glm::vec3 nu = glm::normalize(rotateAbout(u, axis, rot));
    glm::vec3 nf = glm::normalize(rotateAbout(f, axis, rot)); //carry the look along so the view doesn't jump
    commitBasis(nf, nu);
}
void Camera3D::setExtraRotVec(Vec3f extraRotVec) {
    Camera3D::extraRotVec = extraRotVec;
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
    cMatrix = buildViewProj(pos, getEffectiveRot());
}
glm::mat4 Camera3D::buildViewProj(Vec3f pos, Vec3f effRot) const
{
    //Calculate 'rolledUp' from spherical-coordinate-derived natural up
    glm::vec3 forward = glm::vec3(effRot);
    glm::vec3 rolledUp = computeRolledUp(computeNaturalUp(), forward);
    glm::mat4 view = glm::lookAt((glm::vec3)pos, (glm::vec3)(pos+effRot), rolledUp);
    glm::mat4 proj = glm::perspective(glm::radians(fov), ((float)sdlWinW/(float)sdlWinH), nearPlane, farPlane);
    return proj*view;
}
void Camera3D::updateRegAndSubPos() const
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
    float dmx = 0, dmy = 0;
    int mx = Input::getMouseX();
    int my = Input::getMouseY();
    if(mx!=sdlWinW/2 || my!=sdlWinH/2) {
        int lmx = mx;
        int lmy = my;
        SDL_WarpMouseInWindow(sdlWin, sdlWinW/2, sdlWinH/2);
        dmx = sdlWinW/2-lmx;
        dmy = sdlWinH/2-lmy;
        if(ticksSinceLastUnfocus<3) {
            dmx = 0;
            dmy = 0;
        }
    }

    if(freeOrient) {
        //Run every subtick (even with no mouse delta) so externally-set up changes (rollStep/alignUpTo) apply.
        subtickFreeOrientation(dmx, dmy);
    } else {
        yaw -= dmx * sensitivity;
        pitch -= dmy * sensitivity;
        setRot(yaw, pitch, roll);
    }
}
void Camera3D::subtickFreeOrientation(float dmx, float dmy)
{
    glm::vec3 f = glm::normalize(glm::vec3(rotVec));
    glm::vec3 u = glm::normalize(glm::vec3(up));

    //Yaw: rotate forward about the up axis (up unchanged). Sign matches legacy "yaw -= dmx" at up=+Y.
    float yawRad = dmx * sensitivity * (float)nch::rad_deg;
    f = glm::normalize(rotateAbout(f, u, yawRad));

    //Pitch: rotate forward within the plane spanned by (f,u), about right = normalize(f × u). The angle
    //a=angle(f,u) changes by exactly the rotation amount, so clamp a off the up axis to hard-stop the
    //look at vertical (classic FPS). Tumbling onto adjacent surfaces is a separate explicit action
    //(see tumbleBy) — mouse-look never carries 'up' past the clamp.
    glm::vec3 right = glm::cross(f, u);
    float rlen = glm::length(right);
    if(rlen>1e-5f) {
        right /= rlen;
        float pitchRad = dmy * sensitivity * (float)nch::rad_deg; //+dmy (mouse up) rotates the look toward up
        float a = std::acos(glm::clamp(glm::dot(f, u), -1.0f, 1.0f)); //angle from up, in (0, pi)
        const float eps = 0.0174533f; //~1° guard so right never degenerates
        float aClamped = glm::clamp(a - pitchRad, eps, (float)nch::pi - eps);
        float fAllowed = a - aClamped;       //rotate f about right by this to land a exactly at the clamp
        f = glm::normalize(rotateAbout(f, right, fAllowed));
    }

    commitBasis(f, u);
}
void Camera3D::commitBasis(glm::vec3 forward, glm::vec3 upv)
{
    forward = glm::normalize(forward);
    upv = glm::normalize(upv);
    //Derive yaw/pitch from forward (same spherical convention as setRot), then the roll about forward
    //that carries the camera's natural-up onto our target up (mirrors StarMath::rollAngleAroundForward).
    float pitchDeg = std::acos(glm::clamp(forward.y, -1.0f, 1.0f)) * (float)nch::deg_rad;
    float yawDeg = std::atan2(forward.z, forward.x) * (float)nch::deg_rad;
    float pr = pitchDeg*(float)nch::rad_deg, yr = yawDeg*(float)nch::rad_deg;
    glm::vec3 natUp(-std::cos(pr)*std::cos(yr), std::sin(pr), -std::cos(pr)*std::sin(yr));
    glm::vec3 a = natUp - forward*glm::dot(natUp, forward);
    glm::vec3 b = upv - forward*glm::dot(upv, forward);
    float rollDeg = 0.0f;
    float la = glm::length(a), lb = glm::length(b);
    if(la>1e-6f && lb>1e-6f) {
        a /= la; b /= lb;
        float cosA = glm::clamp(glm::dot(a, b), -1.0f, 1.0f);
        float sinA = glm::dot(glm::cross(a, b), forward);
        rollDeg = std::atan2(sinA, cosA) * (float)nch::deg_rad;
    }
    setRot(yawDeg, pitchDeg, rollDeg);
    up = Vec3f(upv.x, upv.y, upv.z); //setRot leaves 'up' alone; keep it as the live up axis
}
glm::vec3 Camera3D::rotateAbout(glm::vec3 v, glm::vec3 axis, float angRad)
{
    return glm::vec3(glm::rotate(glm::mat4(1.0f), angRad, axis) * glm::vec4(v, 0.0f));
}
glm::vec3 Camera3D::forwardInBodyFrame() const
{
    //Undo the minimal-arc rotation that maps model +Y onto 'up' (the same rotation the player mob applies
    //as its base rotation), i.e. rotate forward by the arc that takes 'up' back onto +Y. This expresses the
    //look direction in the body's own frame, so a derived yaw/pitch matches how a tilted body actually faces.
    glm::vec3 f = glm::normalize(glm::vec3(rotVec));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    glm::vec3 m(0.0f, 1.0f, 0.0f);
    glm::vec3 axis = glm::cross(u, m);
    float s = glm::length(axis);
    float c = glm::clamp(glm::dot(u, m), -1.0f, 1.0f);
    if(s<1e-6f) {
        if(c>=0.0f) return f;                   //up already +Y: body frame == world frame
        return glm::vec3(f.x, -f.y, -f.z);      //up == -Y: 180° about +X (matches the mob's base rotation)
    }
    return rotateAbout(f, axis/s, std::atan2(s, c));
}
glm::vec3 Camera3D::computeNaturalUp() const
{
    //Derived from spherical coordinate partial derivative w.r.t. pitch.
    //Always perpendicular to rotVec, so lookAt never degenerates at the poles.
    float pitchRad = pitch*(float)nch::rad_deg;
    float yawRad = yaw*(float)nch::rad_deg;
    return glm::vec3(
        -std::cos(pitchRad)*std::cos(yawRad),
         std::sin(pitchRad),
        -std::cos(pitchRad)*std::sin(yawRad)
    );
}
glm::vec3 Camera3D::computeRolledUp(glm::vec3 naturalUp, glm::vec3 forward) const
{
    float rollRad = roll*(float)nch::rad_deg;
    return glm::rotate(glm::mat4(1.0f), rollRad, forward)*glm::vec4(naturalUp, 0.0f);
}
