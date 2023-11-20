#include <XPLMPlugin.h>
#include <XPLMMenus.h>
#include <XPLMUtilities.h>
#include <XPLMProcessing.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>

#include "main.hpp"
#include "serial.hpp"
#include "ui.hpp"
#include "telemetry.hpp"
#include "calibration.hpp"

State state;
void SetState(State new_state) {state = new_state;}
State GetState() {return state;}

PLUGIN_API int XPluginStart(
    char *outName,
    char *outSig,
    char *outDesc) {

    strcpy(outName, "Ardupilot HITL");
    strcpy(outSig, "https://github.com/qgerman2/xplane-HITL");
    strcpy(outDesc, "A plugin to enable simple HITL testing with Ardupilot");

    UI::Menu::Create();
    UI::Window::Create();

    XPLMRegisterFlightLoopCallback(Loop, -1, NULL);

    return 1;
}

PLUGIN_API void	XPluginStop(void) {

}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void *inParam) {

}

float Loop(
    float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
    int inCounter, void *inRefcon) {
    switch (state) {
        case State::Telemetry:
            if (Serial::IsOpen()) {
                Telemetry::UpdateState();
                Telemetry::ProcessState();
                Serial::Poll();
            }
            break;
        case State::Calibration:
            Calibration::Loop();
            break;
    }
    return -1.0;
}