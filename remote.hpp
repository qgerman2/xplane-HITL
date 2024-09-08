#pragma once
#include <utility>

namespace Remote {
    void SetOverride(bool state);
    void Receive();
    void UpdateDataRefs();
    void StartIgnition();
    void StopIgnition();
}

// https://rosettacode.org/wiki/Map_range#C++
template<typename tVal>
tVal map_value(std::pair<tVal, tVal> a, std::pair<tVal, tVal> b, tVal inVal) {
    tVal inValNorm = inVal - a.first;
    tVal aUpperNorm = a.second - a.first;
    tVal normPosition = inValNorm / aUpperNorm;

    tVal bUpperNorm = b.second - b.first;
    tVal bValNorm = normPosition * bUpperNorm;
    tVal outVal = b.first + bValNorm;

    return outVal;
}