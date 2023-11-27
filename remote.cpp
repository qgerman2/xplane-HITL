#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
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
    }
    struct {
        char header[4] = { 'H', 'I', 'T', 'L' };
        float aileron;
        float elevator;
        float rudder;
        float throttle;
    } servo_msg;
    uint8_t buffer[sizeof(servo_msg)];
    int pos = 0;
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
    XPLMSetDataf(DataRef::roll, servo_msg.aileron);
    XPLMSetDataf(DataRef::pitch, servo_msg.elevator);
    XPLMSetDataf(DataRef::yaw, servo_msg.rudder);
    XPLMSetDatavf(DataRef::throttle, &servo_msg.throttle, 0, 1);
}