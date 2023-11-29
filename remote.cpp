#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <algorithm>
#include "remote.hpp"
#include "serial.hpp"

namespace Remote {
    namespace DataRef {
        XPLMDataRef override_roll = XPLMFindDataRef("sim/operation/override/override_joystick_roll");
        XPLMDataRef override_pitch = XPLMFindDataRef("sim/operation/override/override_joystick_pitch");
        XPLMDataRef override_yaw = XPLMFindDataRef("sim/operation/override/override_joystick_heading");
        XPLMDataRef override_throttle = XPLMFindDataRef("sim/operation/override/override_throttles");
        XPLMDataRef roll = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
        XPLMDataRef pitch = XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
        XPLMDataRef yaw = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");
        XPLMDataRef throttle = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
        XPLMDataRef brake = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");
    }
    struct {
        char header[4] = { 'H', 'I', 'T', 'L' };
        bool armed;
        float aileron;
        float elevator;
        float rudder;
        float throttle;
    } servo_msg;
    uint8_t buffer[sizeof(servo_msg)];
    int pos = 0;
    bool was_armed = false;
    void Poll();
    void Update();
}

void Remote::Enable() {
    XPLMSetDatai(DataRef::override_roll, 1);
    XPLMSetDatai(DataRef::override_pitch, 1);
    XPLMSetDatai(DataRef::override_yaw, 1);
    XPLMSetDatai(DataRef::override_throttle, 1);
}

void Remote::Disable() {
    XPLMSetDatai(DataRef::override_roll, 0);
    XPLMSetDatai(DataRef::override_pitch, 0);
    XPLMSetDatai(DataRef::override_yaw, 0);
    XPLMSetDatai(DataRef::override_throttle, 0);
}

void Remote::Loop() {
    Poll();
}

void Remote::Poll() {
    int nbytes = Serial::Available();
    while (nbytes-- > 0) {
        // read a single byte into the buffer
        if (!Serial::Read(&buffer[pos])) {
            continue;
        }
        // check if the first bytes of the message match predefined header
        if (pos < sizeof(servo_msg.header)) {
            if (buffer[pos] != servo_msg.header[pos]) {
                pos = 0;
                continue;
            }
        }
        // if we get to this point it means the first bytes have matched the
        // header so far
        pos++;
        if (pos == sizeof(servo_msg)) {
            memcpy(&servo_msg, buffer, sizeof(servo_msg));
            pos = 0;
            Update();
        }
    }
}

void Remote::Update() {
    if (!was_armed && servo_msg.armed) {
        XPLMSetDataf(DataRef::brake, 0);
    }
    XPLMSetDataf(DataRef::roll, std::clamp(servo_msg.aileron, -1.0f, 1.0f));
    XPLMSetDataf(DataRef::pitch, std::clamp(servo_msg.elevator, -1.0f, 1.0f));
    XPLMSetDataf(DataRef::yaw, std::clamp(servo_msg.rudder, -1.0f, 1.0f));
    float throttle[16];
    std::fill_n(throttle, 16, std::clamp(servo_msg.throttle, 0.0f, 1.0f));
    XPLMSetDatavf(DataRef::throttle, throttle, 0, 16);
    was_armed = servo_msg.armed;
}