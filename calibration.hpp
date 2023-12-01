#pragma once

#include <array>
#include <string>

namespace Calibration {
    inline std::array<std::string, 6> steps = {
        "Level",
        "Roll left",
        "Roll right",
        "Nose down",
        "Nose up",
        "Back"
    };
    void NextCalibrationStep();
    void PreviousCalibrationStep();
    void Loop(float dt);
    bool IsEnabled();
    bool Toggle();
    void ToggleRotation();
}