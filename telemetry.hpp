#pragma once

#define GRAVITY_MSS 9.80665f

namespace Telemetry {
    void Loop(float dt);
    void Reset();
}