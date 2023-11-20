#pragma once

enum class State {
    Telemetry,
    Calibration
};

void SetState(State state);
State GetState();

float Loop(
    float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
    int inCounter, void *inRefcon);