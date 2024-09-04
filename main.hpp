#pragma once

const char header[4] = { 'H', 'I', 'T', 'L' };

float Loop(
    float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
    int inCounter, void *inRefcon);