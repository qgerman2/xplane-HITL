#include <Eigen/Geometry>
#include <string>
#include <tweeny-3.2.0.h>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>

#include <array>
#include <cmath>
#include <format>

#include "calibration.hpp"
#include "util.hpp"

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
        XPLMDataRef quat = XPLMFindDataRef("sim/flightmodel/position/q");
        XPLMDataRef P = XPLMFindDataRef("sim/flightmodel/position/P");
        XPLMDataRef Q = XPLMFindDataRef("sim/flightmodel/position/Q");
        XPLMDataRef R = XPLMFindDataRef("sim/flightmodel/position/R");
    }
    namespace Saved {
        Eigen::Vector3d pos;
        Eigen::Quaternionf rot;
        Eigen::Vector3f vel;
        Eigen::Vector3f rates;
    }
    namespace Anim {
        int millis = 0;
        Eigen::Vector3d start_pos;
        Eigen::Quaternionf start_rot;
        Eigen::Vector3d last_pos;
        Eigen::Quaternionf last_rot;
        Eigen::Vector3d dest_pos;
        Eigen::Quaternionf dest_rot;
        tweeny::tween<float> tween;
        void CreateTween();
    }
    int step = 0;
    int enabled = 0;
    bool IsEnabled() { return enabled; };
}

void Calibration::Anim::CreateTween() {
    millis = 0;
    Eigen::Vector3f euler = QuatToEuler(Saved::rot);
    switch (step) {
    case -1: // Return to saved
        dest_pos = Saved::pos;
        dest_rot = Saved::rot;
        break;
    case 0: // Accelerometer - Front
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ 0, 0, euler.z() });
        break;
    case 1: // Accelerometer - Roll left
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ -90, 0, euler.z() });
        break;
    case 2: // Accelerometer - Roll right
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ 90, 0, euler.z() });
        break;
    case 3: // Accelerometer - Turn left
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ 0, 0, euler.z() - 90 });
        break;
    case 4: // Accelerometer - Turn right
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ 0, 0, euler.z() + 90 });
        break;
    case 5: // Accelerometer - Upside down
        dest_pos = Saved::pos + Eigen::Vector3d{ 0, 5, 0 };
        dest_rot = EulerToQuat({ 180, 0, euler.z() });
        break;
    }
    tween = tweeny::from(0.0f).to(1.0f).during(1000)
        .via(tweeny::easing::quadraticInOutEasing());
    start_pos = last_pos;
    start_rot = last_rot;
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
    XPLMGetDatavf(DataRef::quat, &Saved::rot.w(), 0, 1);
    XPLMGetDatavf(DataRef::quat, Saved::rot.vec().data(), 1, 3);
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
    Anim::last_pos = Saved::pos;
    Anim::last_rot = Saved::rot;
    step = 0;
    Anim::CreateTween();
    // Disable plane physics
    enabled = 1;
    //XPLMSetDatavi(DataRef::physics, &enabled, 0, 1);
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

void Calibration::Loop(float dt) {
    using namespace Anim;
    millis += dt * 1000;
    float t = tween.seek(millis);
    Eigen::Vector3d curr_pos = {
        start_pos.x() + (dest_pos.x() - start_pos.x()) * double(t),
        start_pos.y() + (dest_pos.y() - start_pos.y()) * double(t),
        start_pos.z() + (dest_pos.z() - start_pos.z()) * double(t),
    };
    Eigen::Quaternionf curr_rot = start_rot.slerp(t, dest_rot);
    XPLMSetDatad(DataRef::x, curr_pos.x());
    XPLMSetDatad(DataRef::y, curr_pos.y());
    XPLMSetDatad(DataRef::z, curr_pos.z());
    XPLMSetDataf(DataRef::vx, (curr_pos.x() - last_pos.x()) / dt);
    XPLMSetDataf(DataRef::vy, (curr_pos.y() - last_pos.y()) / dt);
    XPLMSetDataf(DataRef::vz, (curr_pos.z() - last_pos.z()) / dt);
    XPLMSetDatavf(DataRef::quat, curr_rot.vec().data(), 1, 3);
    XPLMSetDatavf(DataRef::quat, &curr_rot.w(), 0, 1);
    Eigen::Vector3f rates = angularVelocity(last_rot, curr_rot, dt);
    XPLMSetDataf(DataRef::P, rates.x());
    XPLMSetDataf(DataRef::Q, rates.y());
    XPLMSetDataf(DataRef::R, rates.z());
    last_pos = curr_pos;
    last_rot = curr_rot;
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
        XPLMSetDatavf(DataRef::quat, Saved::rot.vec().data(), 1, 3);
        XPLMSetDatavf(DataRef::quat, &Saved::rot.w(), 0, 1);
        // Enable plane physics
        enabled = 0;
        //XPLMSetDatavi(DataRef::physics, &enabled, 0, 1);
    }
}