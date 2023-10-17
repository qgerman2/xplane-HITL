#pragma once

namespace Sim {
    float Loop(
        float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
        int inCounter, void *inRefcon);
}