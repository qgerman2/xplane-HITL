#include <XPLMDataAccess.h>
#include "calibration.hpp"

namespace Calibration {
    Step& operator++(Step& s) {
        if (s == Step::AccelerometerFlip) {
            return s = Step::AccelerometerFront;
        } else {
            return s = static_cast<Step>(static_cast<int>(s)+1);
        }
    }
    namespace DataRef {
        XPLMDataRef physics = XPLMFindDataRef("sim/operation/override/override_planepath");
        XPLMDataRef x = XPLMFindDataRef("sim/flightmodel/position/local_x");
        XPLMDataRef y = XPLMFindDataRef("sim/flightmodel/position/local_y");
        XPLMDataRef z = XPLMFindDataRef("sim/flightmodel/position/local_z");
        
    }
}

void Calibration::Loop() {
    int disable[1] = {1};
    XPLMSetDatavi(DataRef::physics, &disable[0], 0, 1);
}