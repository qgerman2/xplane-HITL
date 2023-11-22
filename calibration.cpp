#include <Eigen/Geometry>
#include <string>
#include <tweeny-3.2.0.h>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>

#include <array>
#include <cmath>

#include "calibration.hpp"

namespace Calibration {
    namespace DataRef {
        XPLMDataRef physics = XPLMFindDataRef("sim/operation/override/override_planepath");
        XPLMDataRef x = XPLMFindDataRef("sim/flightmodel/position/local_x");
        XPLMDataRef y = XPLMFindDataRef("sim/flightmodel/position/local_y");
        XPLMDataRef z = XPLMFindDataRef("sim/flightmodel/position/local_z");
        XPLMDataRef vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
        XPLMDataRef vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
        XPLMDataRef vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");
        XPLMDataRef roll = XPLMFindDataRef("sim/flightmodel/position/phi");
        XPLMDataRef pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
        XPLMDataRef yaw = XPLMFindDataRef("sim/flightmodel/position/psi");
        XPLMDataRef P = XPLMFindDataRef("sim/flightmodel/position/P");
        XPLMDataRef Q = XPLMFindDataRef("sim/flightmodel/position/Q");
        XPLMDataRef R = XPLMFindDataRef("sim/flightmodel/position/R");
    }
    namespace Saved {
        Eigen::Vector3d pos;
        Eigen::Vector3f rot;
        Eigen::Vector3f vel;
        Eigen::Vector3f rates;
    }
    namespace Anim {
        int millis = 0;
        Eigen::Vector3d prev_pos;
        Eigen::Vector3f prev_rot;
        Eigen::Vector3d curr_pos;
        Eigen::Vector3f curr_rot;
        Eigen::Vector3d next_pos;
        Eigen::Vector3f next_rot;
        tweeny::tween<double, double, double> pos_tween;
        tweeny::tween<float, float, float> rot_tween;
        void CreateTween();
    }
    int step = 0;
    int enabled = 0;
    bool IsEnabled() { return enabled; };
}

void Calibration::Anim::CreateTween() {
    millis = 0;
    switch (step) {
    case -1: // Return to saved
        next_pos = Saved::pos;
        next_rot = Saved::rot;
        break;
    case 0: // Accelerometer - Front
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 1, 0 };
        next_rot = { 0, 0, Saved::rot.z() };
        break;
    case 1: // Accelerometer - Roll right
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 3, 0 };
        next_rot = { 90, 0, Saved::rot.z() };
        break;
    case 2: // Accelerometer - Roll left
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 3, 0 };
        next_rot = { -90, 0, Saved::rot.z() };
        break;
    case 3: // Accelerometer - Turn right
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 1, 0 };
        next_rot = { 0, 0, Saved::rot.z() + 90 };
        break;
    case 4: // Accelerometer - Turn left
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 1, 0 };
        next_rot = { 0, 0, Saved::rot.z() - 90 };
        break;
    case 5: // Accelerometer - Upside down
        next_pos = Saved::pos + Eigen::Vector3d{ 0, 2, 0 };
        next_rot = { 180, 0, Saved::rot.z() };
        break;
    }
    pos_tween = tweeny::from(prev_pos.x(), prev_pos.y(), prev_pos.z())
        .to(next_pos.x(), next_pos.y(), next_pos.z())
        .during(1000)
        .via(tweeny::easing::quadraticInOutEasing());
    rot_tween = tweeny::from(prev_rot.x(), prev_rot.y(), prev_rot.z())
        .to(next_rot.x(), next_rot.y(), next_rot.z())
        .during(1000)
        .via(tweeny::easing::quadraticInOutEasing());
    curr_pos = prev_pos;
    curr_rot = prev_rot;
}

void Calibration::NextCalibrationStep() {
    if (step == -1) { return; };
    step = (step + 1) % steps.size();
    Anim::CreateTween();
};
void Calibration::PreviousCalibrationStep() {
    if (step == -1) { return; };
    step = step > 0 ? step - 1 : steps.size() - 1;
    Anim::CreateTween();
};

void Calibration::Enable() {
    // Save plane position and velocity for later
    Saved::pos = {
        XPLMGetDatad(DataRef::x),
        XPLMGetDatad(DataRef::y),
        XPLMGetDatad(DataRef::z)
    };
    Saved::rot = {
        XPLMGetDataf(DataRef::roll),
        XPLMGetDataf(DataRef::pitch),
        XPLMGetDataf(DataRef::yaw)
    };
    Saved::vel = {
        XPLMGetDataf(DataRef::vx),
        XPLMGetDataf(DataRef::vy),
        XPLMGetDataf(DataRef::vz)
    };
    Saved::rates = {
        XPLMGetDataf(DataRef::P),
        XPLMGetDataf(DataRef::Q),
        XPLMGetDataf(DataRef::R)
    };
    // Set up animation
    // Position and rotation for animation
    Anim::prev_pos = Saved::pos;
    Anim::prev_rot = {
        XPLMGetDataf(DataRef::roll),
        XPLMGetDataf(DataRef::pitch),
        XPLMGetDataf(DataRef::yaw)
    };
    step = 0;
    Anim::CreateTween();
    // Disable plane physics
    enabled = 1;
    XPLMSetDatavi(DataRef::physics, &enabled, 0, 1);
    XPLMSetDataf(DataRef::vx, 0);
    XPLMSetDataf(DataRef::vy, 0);
    XPLMSetDataf(DataRef::vz, 0);
    XPLMSetDataf(DataRef::P, 0);
    XPLMSetDataf(DataRef::Q, 0);
    XPLMSetDataf(DataRef::R, 0);
};
void Calibration::Disable() {
    if (step != -1) {
        step = -1;
        Anim::CreateTween();
    }
};

float constrain(float angle) {
    float result = std::fmod(angle, 360.0f);
    if (result < 0) { result += 360; };
    return result;
}

void Calibration::Loop(float dt) {
    using namespace Anim;
    millis += dt * 1000;
    prev_pos = curr_pos;
    prev_rot = curr_rot;
    std::array<double, 3> new_pos = pos_tween.seek(millis);
    std::array<float, 3> new_rot = rot_tween.seek(millis);
    curr_pos = { new_pos[0], new_pos[1], new_pos[2] };
    curr_rot = { new_rot[0], new_rot[1], new_rot[2] };
    XPLMSetDatad(DataRef::x, curr_pos.x());
    XPLMSetDatad(DataRef::y, curr_pos.y());
    XPLMSetDatad(DataRef::z, curr_pos.z());
    XPLMSetDataf(DataRef::roll, constrain(curr_rot.x()));
    XPLMSetDataf(DataRef::pitch, constrain(curr_rot.y()));
    XPLMSetDataf(DataRef::yaw, constrain(curr_rot.z()));
    XPLMSetDataf(DataRef::vx, (curr_pos.x() - prev_pos.x()) / dt);
    XPLMSetDataf(DataRef::vy, (curr_pos.y() - prev_pos.y()) / dt);
    XPLMSetDataf(DataRef::vz, (curr_pos.z() - prev_pos.z()) / dt);
    XPLMSetDataf(DataRef::P, (curr_rot.x() - prev_rot.x()) / dt);
    XPLMSetDataf(DataRef::Q, (curr_rot.y() - prev_rot.y()) / dt);
    XPLMSetDataf(DataRef::R, (curr_rot.z() - prev_rot.z()) / dt);
    if (step == -1 && millis >= 1000) {
        // Restore saved position and velocity variables
        XPLMSetDatad(DataRef::x, Saved::pos.x());
        XPLMSetDatad(DataRef::y, Saved::pos.y());
        XPLMSetDatad(DataRef::z, Saved::pos.z());
        XPLMSetDataf(DataRef::vx, Saved::vel.x());
        XPLMSetDataf(DataRef::vy, Saved::vel.y());
        XPLMSetDataf(DataRef::vz, Saved::vel.z());
        XPLMSetDataf(DataRef::P, Saved::rates.x());
        XPLMSetDataf(DataRef::Q, Saved::rates.y());
        XPLMSetDataf(DataRef::R, Saved::rates.z());
        // Enable plane physics
        enabled = 0;
        XPLMSetDatavi(DataRef::physics, &enabled, 0, 1);
    }
}