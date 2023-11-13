#pragma once

#define GRAVITY_MSS 9.80665f

namespace Sim {
    float Loop(
        float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
        int inCounter, void *inRefcon);
}