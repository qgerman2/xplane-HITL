#pragma once

namespace Calibration {
    enum class Step {
        AccelerometerFront,
        AccelerometerRight,
        AccelerometerLeft,
        AccelerometerFlip
    };
    void NextCalibrationStep();
    void PreviousCalibrationStep();
    void Loop();
}