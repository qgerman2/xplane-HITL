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
        XPLMDataRef yaw = XPLMFindDataRef("sim/flightmodel/position/psi");
        XPLMDataRef quat = XPLMFindDataRef("sim/flightmodel/position/q");
        XPLMDataRef P = XPLMFindDataRef("sim/flightmodel/position/P");
        XPLMDataRef Q = XPLMFindDataRef("sim/flightmodel/position/Q");
        XPLMDataRef R = XPLMFindDataRef("sim/flightmodel/position/R");
        XPLMDataRef semilen = XPLMFindDataRef("sim/aircraft/parts/acf_semilen_JND");
    }
    namespace Saved {
        Eigen::Vector3d pos;
        float yaw;
        Eigen::Quaternionf rot;
        Eigen::Vector3f vel;
        Eigen::Vector3f rates;
    }
    namespace Anim {
        int step = 0;
        bool rotate = false;
        int millis = 0;
        float semilen = 0;
        Eigen::Vector3d start_pos;
        Eigen::Quaternionf start_rot;
        Eigen::Vector3d last_pos;
        Eigen::Quaternionf last_rot;
        Eigen::Vector3d dest_pos;
        Eigen::Quaternionf dest_rot;
        tweeny::tween<float> tween;
        tweeny::tween<float> tween_rot;
        void Start();
    }
    int enabled = 0;
    bool IsEnabled() { return enabled; };
    void Save();
    void Restore();
    constexpr int animation_time = 1500; //ms
}

bool Calibration::Toggle() {
    if (!enabled) {
        Save();
        float s[56];
        XPLMGetDatavf(DataRef::semilen, s, 0, 56);
        Anim::semilen = *std::max_element(s, s + 56);
        Anim::last_pos = Saved::pos;
        Anim::last_rot = Saved::rot;
        Anim::step = 0;
        Anim::Start();
        enabled = 1;
        return true;
    } else {
        if (Anim::step != -1) {
            Anim::step = -1;
            Anim::rotate = false;
            Anim::Start();
        }
        return false;
    }
}

void Calibration::ToggleRotation() {
    if (!enabled || Anim::step == -1) { return; };
    Anim::rotate = !Anim::rotate;
}

void Calibration::NextCalibrationStep() {
    if (!enabled || Anim::step == -1) { return; };
    Anim::step = (Anim::step + 1) % steps.size();
    Anim::Start();
};
void Calibration::PreviousCalibrationStep() {
    if (!enabled || Anim::step == -1) { return; };
    Anim::step = Anim::step > 0 ? Anim::step - 1 : steps.size() - 1;
    Anim::Start();
};

void Calibration::Save() {
    // Save plane position and velocity for later
    Saved::pos = {
        XPLMGetDatad(DataRef::x),
        XPLMGetDatad(DataRef::y),
        XPLMGetDatad(DataRef::z)
    };
    Saved::yaw = XPLMGetDataf(DataRef::yaw);
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
};

void Calibration::Restore() {
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
}

void Calibration::Anim::Start() {
    millis = 0;
    Eigen::Vector3d base_pos = Saved::pos + Eigen::Vector3d{ 0, semilen, 0 };
    Eigen::Quaternionf base_rot = Eigen::Quaternionf(1, 0, 0, 0) *
        Eigen::AngleAxisf(Saved::yaw * deg_to_rad, Eigen::Vector3f::UnitZ());
    switch (step) {
    case -1: // Return to saved
        dest_pos = Saved::pos;
        dest_rot = Saved::rot;
        break;
    case 0: // Accelerometer - Front
        dest_pos = base_pos;
        dest_rot = base_rot;
        break;
    case 1: // Accelerometer - Roll left
        dest_pos = base_pos;
        dest_rot = base_rot * Eigen::AngleAxisf(-M_PI / 2, Eigen::Vector3f::UnitX());
        break;
    case 2: // Accelerometer - Roll right
        dest_pos = base_pos;
        dest_rot = base_rot * Eigen::AngleAxisf(M_PI / 2, Eigen::Vector3f::UnitX());
        break;
    case 3: // Accelerometer - Nose down
        // if the plane is perfectly nose down/up it glitches
        // a small offset prevents it
        dest_pos = base_pos;
        dest_rot = base_rot * Eigen::AngleAxisf(-M_PI / 2 + 0.0001f, Eigen::Vector3f::UnitY());
        break;
    case 4: // Accelerometer - Nose up
        dest_pos = base_pos;
        dest_rot = base_rot * Eigen::AngleAxisf(M_PI / 2 + 0.0001f, Eigen::Vector3f::UnitY());
        break;
    case 5: // Accelerometer - Upside down
        dest_pos = base_pos;
        dest_rot = base_rot * Eigen::AngleAxisf(M_PI, Eigen::Vector3f::UnitX());
        break;
    }
    tween = tweeny::from(0.0f).to(1.0f).during(animation_time)
        .via(tweeny::easing::quadraticInOutEasing());
    start_pos = last_pos;
    start_rot = last_rot;
}


void Calibration::Loop(float dt) {
    using namespace Anim;
    // update animation timer
    if (millis < animation_time || rotate) {
        millis += dt * 1000;
        if (millis > animation_time && !rotate) {
            millis = animation_time;
        }
    }
    float t = tween.seek(millis);
    // tween position and rotation
    Eigen::Vector3d curr_pos = {
        start_pos.x() + (dest_pos.x() - start_pos.x()) * double(t),
        start_pos.y() + (dest_pos.y() - start_pos.y()) * double(t),
        start_pos.z() + (dest_pos.z() - start_pos.z()) * double(t),
    };
    Eigen::Quaternionf curr_rot = start_rot.slerp(t, dest_rot);
    if (millis > animation_time) {
        float angle = 2 * M_PI / 10000.0f * (millis - animation_time);
        Eigen::Vector3f up = { 0, 0, 1 };
        Eigen::AngleAxisf yaw_rotate = { angle, curr_rot.conjugate() * up };
        curr_rot = dest_rot * yaw_rotate;
    }
    // update data refs
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
    // update last position for next loop iteration
    last_pos = curr_pos;
    last_rot = curr_rot;
    // end calibration if deactivation animation has finished
    if (step == -1 && millis >= animation_time) {
        Restore();
        enabled = 0;
    }
}