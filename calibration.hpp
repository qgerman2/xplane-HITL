#pragma once

#include <array>
#include <string>

namespace Calibration {
    inline std::array<std::string, 6> steps = {
        "Accelerometer - Front",
        "Accelerometer - Roll right",
        "Accelerometer - Roll left",
        "Accelerometer - Turn right",
        "Accelerometer - Turn left",
        "Accelerometer - Upside down"
    };
    void NextCalibrationStep();
    void PreviousCalibrationStep();
    void Loop(float dt);
    void Enable();
    void Disable();
    bool IsEnabled();
}