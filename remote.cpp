#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <algorithm>
#include <format>
#include "remote.hpp"
#include "serial.hpp"
#include "ui.hpp"

namespace Remote {
    namespace DataRef {
        XPLMDataRef override_roll = XPLMFindDataRef("sim/operation/override/override_joystick_roll");
        XPLMDataRef override_pitch = XPLMFindDataRef("sim/operation/override/override_joystick_pitch");
        XPLMDataRef override_yaw = XPLMFindDataRef("sim/operation/override/override_joystick_heading");
        XPLMDataRef override_throttle = XPLMFindDataRef("sim/operation/override/override_throttles");
        XPLMDataRef override_prop_pitch = XPLMFindDataRef("sim/operation/override/override_prop_pitch");
        XPLMDataRef roll = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
        XPLMDataRef pitch = XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
        XPLMDataRef yaw = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");
        XPLMDataRef throttle = XPLMFindDataRef("sim/flightmodel/engine/ENGN_thro_use");
        XPLMDataRef brake = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");
        XPLMDataRef prop_pitch = XPLMFindDataRef("sim/flightmodel/engine/POINT_pitch_deg_use");

        XPLMDataRef max_prop_pitch = XPLMFindDataRef("sim/aircraft/prop/acf_max_pitch");
        XPLMDataRef min_prop_pitch = XPLMFindDataRef("sim/aircraft/prop/acf_min_pitch");
    }

    struct {
        char preamble[4] = { 'H', 'I', 'T', 'L' };
        int type;
    } header;

    struct {
        uint8_t state;
        uint32_t ahrs_count;
    } state_msg;

    struct {
        float roll_cyclic;
        float pitch_cyclic;
        float collective;
        float tail;
        float throttle;
    } heli_msg;

    struct {
        float roll;
        float pitch;
        float yaw;
        float throttle;
    } plane_msg;

    int pos = 0;
    uint8_t buffer[300];

    int state = -1;
    bool override_joy = true;
    void OnState();
    void OnHeli();
}

void Remote::Enable() {
    if (override_joy) {
        XPLMSetDatai(DataRef::override_roll, 1);
        XPLMSetDatai(DataRef::override_pitch, 1);
        XPLMSetDatai(DataRef::override_yaw, 1);
        XPLMSetDatai(DataRef::override_throttle, 1);
        XPLMSetDatai(DataRef::override_prop_pitch, 1);
    }
}

void Remote::Disable() {
    if (!override_joy) {
        XPLMSetDatai(DataRef::override_roll, 0);
        XPLMSetDatai(DataRef::override_pitch, 0);
        XPLMSetDatai(DataRef::override_yaw, 0);
        XPLMSetDatai(DataRef::override_throttle, 0);
        XPLMSetDatai(DataRef::override_prop_pitch, 0);
    }
    state = -1;
    UI::Window::LabelRemoteArmed::SetText("None");
}

void Remote::SetOverride(bool state) {
    override_joy = state;
    if (Serial::IsOpen()) {
        if (override_joy) {
            Enable();
        } else {
            Disable();
        }
    }
}

void Remote::Receive() {
    int nbytes = Serial::Available();
    while (nbytes-- > 0) {
        // read a single byte into the buffer
        if (!Serial::Read(&buffer[pos])) {
            continue;
        }
        // check if the first bytes of the message match predefined header
        if (pos < sizeof(header.preamble)) {
            if (buffer[pos] != header.preamble[pos]) {
                pos = 0;
                continue;
            }
        }
        pos++;
        if (pos == sizeof(header)) {
            memcpy(&header, buffer, sizeof(header));
        }
        if (pos > sizeof(header)) {
            if (header.type == 0) {
                pos = 0;
            } else if (header.type == 1) {
                if (pos == sizeof(header) + sizeof(state_msg)) {
                    memcpy(&state_msg, &buffer[sizeof(header)], sizeof(state_msg));
                    OnState();
                    pos = 0;
                };
            } else if (header.type == 2) {
                if (pos == sizeof(header) + sizeof(plane_msg)) {
                    pos = 0;
                }
            } else if (header.type == 3) {
                if (pos == sizeof(header) + sizeof(heli_msg)) {
                    memcpy(&heli_msg, &buffer[sizeof(header)], sizeof(heli_msg));
                    OnHeli();
                    pos = 0;
                }
            } else {
                pos = 0;
            }
        }
    }
}

void Remote::OnState() {
    UI::Window::LabelAHRSCount::SetText(std::format("AHRS: {} Hz", state_msg.ahrs_count));
}

void Remote::OnHeli() {
    if (override_joy) {
        XPLMSetDataf(DataRef::roll, heli_msg.roll_cyclic);
        XPLMSetDataf(DataRef::pitch, heli_msg.pitch_cyclic);

        float max_collective; float min_collective;
        XPLMGetDatavf(DataRef::max_prop_pitch, &max_collective, 0, 1);
        XPLMGetDatavf(DataRef::min_prop_pitch, &min_collective, 0, 1);

        float max_tail; float min_tail;
        XPLMGetDatavf(DataRef::max_prop_pitch, &max_tail, 1, 1);
        XPLMGetDatavf(DataRef::min_prop_pitch, &min_tail, 1, 1);

        float collective = map_value(std::pair(-1.0f, 1.0f), std::pair(min_collective, max_collective), heli_msg.collective);
        float tail = map_value(std::pair(-1.0f, 1.0f), std::pair(min_tail, max_tail), heli_msg.tail);

        XPLMSetDatavf(DataRef::prop_pitch, &collective, 0, 1);
        XPLMSetDatavf(DataRef::prop_pitch, &tail, 1, 1);
        float throttle[8]; std::fill_n(throttle, 8, heli_msg.throttle);
        XPLMSetDatavf(DataRef::throttle, &throttle[0], 0, 8);
    }
}