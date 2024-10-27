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
        XPLMDataRef engine_running = XPLMFindDataRef("sim/flightmodel/engine/ENGN_running");

        XPLMDataRef max_prop_pitch = XPLMFindDataRef("sim/aircraft/prop/acf_max_pitch");
        XPLMDataRef min_prop_pitch = XPLMFindDataRef("sim/aircraft/prop/acf_min_pitch");

        XPLMDataRef governor = XPLMFindDataRef("sim/cockpit2/engine/actuators/governor_on");

        XPLMDataRef fuel_remaining = XPLMFindDataRef("sim/flightmodel/weight/m_fuel_total");
    }
    namespace Commands {
        XPLMCommandRef starter = XPLMFindCommand("sim/operation/auto_start");
        XPLMCommandRef shutdown = XPLMFindCommand("sim/starters/shut_down");
    }

    struct {
        char preamble[4] = { 'H', 'I', 'T', 'L' };
        int type;
    } header;

    struct {
        int len;
        char postamble[3] = { 'E','N','D' };
    } footer;

    enum MSG_TYPE {
        PING,
        STATE,
        PLANE,
        HELI
    };

    struct {
        uint8_t state;
        uint32_t ahrs_count;
        uint16_t starter;
    } state_msg;

    struct {
        uint16_t roll;
        uint16_t pitch;
        uint16_t yaw;
        uint16_t throttle;
    } plane_msg;

    struct {
        uint16_t roll_cyclic;
        uint16_t pitch_cyclic;
        uint16_t collective;
        uint16_t tail;
        uint16_t throttle;
    } heli_msg;

    size_t msg_size[4]{
        0,
        sizeof(state_msg),
        sizeof(plane_msg),
        sizeof(heli_msg)
    };

    int pos = 0;
    uint8_t buffer[400];

    float max_collective = 0;
    float min_collective = 0;
    float max_tail = 0;
    float min_tail = 0;

    std::pair pwm(1100.0f, 1900.0f);

    bool starting = false;

    int state = -1;
    bool override_joy = true;
    void OnState();
    void OnPlane();
    void OnHeli();
}

void Remote::SetOverride(bool state) {
    override_joy = state;
    UpdateDataRefs();
}

void Remote::UpdateDataRefs() {
    if (Serial::IsOpen() and override_joy) {
        XPLMSetDatai(DataRef::override_roll, 1);
        XPLMSetDatai(DataRef::override_pitch, 1);
        XPLMSetDatai(DataRef::override_yaw, 1);
        XPLMSetDatai(DataRef::override_throttle, 1);
        XPLMSetDatai(DataRef::override_prop_pitch, 1);
    } else {
        XPLMSetDatai(DataRef::override_roll, 0);
        XPLMSetDatai(DataRef::override_pitch, 0);
        XPLMSetDatai(DataRef::override_yaw, 0);
        XPLMSetDatai(DataRef::override_throttle, 0);
        XPLMSetDatai(DataRef::override_prop_pitch, 0);
    }
    XPLMGetDatavf(DataRef::max_prop_pitch, &max_collective, 0, 1);
    XPLMGetDatavf(DataRef::min_prop_pitch, &min_collective, 0, 1);
    XPLMGetDatavf(DataRef::max_prop_pitch, &max_tail, 1, 1);
    XPLMGetDatavf(DataRef::min_prop_pitch, &min_tail, 1, 1);
}

void Remote::Update() {
    Receive();
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
        // check if header received is valid
        if (pos == sizeof(header)) {
            memcpy(&header, buffer, sizeof(header));
            if (header.type < 0 || header.type > 3) {
                pos = 0;
                header.type = 0;
                continue;
            }
        }
        // check if there are enough bytes for the packet type
        if (pos == sizeof(header) + msg_size[header.type] + sizeof(footer)) {
            pos = 0;
            // check footer
            memcpy(&footer, &buffer[sizeof(header) + msg_size[header.type]], sizeof(footer));
            if (footer.len != sizeof(header) + msg_size[header.type]) { break; };
            if (strncmp(footer.postamble, "END", 3) != 0) { break; };
            // process message
            switch (header.type) {
            case PING:
                break;
            case STATE:
                memcpy(&state_msg, &buffer[sizeof(header)], msg_size[header.type]);
                OnState();
                break;
            case PLANE:
                memcpy(&plane_msg, &buffer[sizeof(header)], msg_size[header.type]);
                OnPlane();
                break;
            case HELI:
                memcpy(&heli_msg, &buffer[sizeof(header)], msg_size[header.type]);
                OnHeli();
                break;
            }
        }
    }
}

void Remote::OnState() {
    // armed ui text
    std::string armed = "";
    switch (state_msg.state) {
    case 0:
        armed = "Safety on";
        break;
    case 1:
        armed = "Disarmed";
        break;
    case 2:
        armed = "Armed";
        break;
    }
    UI::Window::LabelRemoteArmed::SetText(armed.c_str());
    // start engine if armed
    // park brake when unarmed
    if (override_joy) {
        if (state_msg.state == 2) {
            XPLMSetDataf(DataRef::brake, 0);
        } else {
            XPLMSetDataf(DataRef::brake, 1);
        }
        // engine handling
        int engine_running;
        XPLMGetDatavi(DataRef::engine_running, &engine_running, 0, 1);
        // if disarmed
        if (state_msg.state != 2) {
            // prop pitch 0 
            float collective = 0;
            float tail = 0;
            XPLMSetDatavf(DataRef::prop_pitch, &collective, 0, 1);
            XPLMSetDatavf(DataRef::prop_pitch, &tail, 1, 1);
            // shutdown engines
            if (engine_running) {
                XPLMCommandOnce(Commands::shutdown);
            };
        } else { // if armed
            float throttle;
            XPLMGetDatavf(DataRef::throttle, &throttle, 0, 1);
            float fuel_remaining = XPLMGetDataf(DataRef::fuel_remaining);
            if (!engine_running && throttle > 0 && fuel_remaining > 0.1) {
                XPLMCommandOnce(Commands::starter);
            }
        }
    }
    // packet rate
    UI::Window::LabelAHRSCount::SetText(std::format("AHRS: {} Hz", state_msg.ahrs_count));
}

void Remote::OnPlane() {
    if (override_joy) {
        XPLMSetDataf(DataRef::roll, map_value(pwm, std::pair(-1.0f, 1.0f), static_cast<float>(plane_msg.roll)));
        XPLMSetDataf(DataRef::pitch, map_value(pwm, std::pair(-1.0f, 1.0f), static_cast<float>(plane_msg.pitch)));
        XPLMSetDataf(DataRef::yaw, map_value(pwm, std::pair(-1.0f, 1.0f), static_cast<float>(plane_msg.yaw)));
        float throttle[8];
        std::fill_n(throttle, 8, map_value(pwm, std::pair(0.0f, 1.0f), static_cast<float>(plane_msg.throttle)));
        XPLMSetDatavf(DataRef::throttle, &throttle[0], 0, 8);
    }
}

void Remote::OnHeli() {
    if (override_joy) {
        XPLMSetDatai(DataRef::governor, 0);
        XPLMSetDataf(DataRef::roll, map_value(pwm, std::pair(-1.0f, 1.0f), static_cast<float>(heli_msg.roll_cyclic)));
        XPLMSetDataf(DataRef::pitch, map_value(pwm, std::pair(-1.0f, 1.0f), static_cast<float>(heli_msg.pitch_cyclic)));

        float collective = map_value(pwm, std::pair(min_collective, max_collective), static_cast<float>(heli_msg.collective));
        float tail = map_value(pwm, std::pair(min_tail, max_tail), static_cast<float>(heli_msg.tail));

        XPLMSetDatavf(DataRef::prop_pitch, &collective, 0, 1);
        XPLMSetDatavf(DataRef::prop_pitch, &tail, 1, 1);

        float throttle[8];
        std::fill_n(throttle, 8, map_value(pwm, std::pair(0.0f, 1.0f), static_cast<float>(heli_msg.throttle)));
        XPLMSetDatavf(DataRef::throttle, &throttle[0], 0, 8);
    }
}