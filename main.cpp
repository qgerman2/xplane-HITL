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
#include "remote.hpp"

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
    Serial::StopScan();
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    Serial::StopScan();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void *inParam) {
    if (inFrom == XPLM_PLUGIN_XPLANE) {
        switch (inMsg) {
        case XPLM_MSG_PLANE_LOADED:
        case XPLM_MSG_AIRPORT_LOADED:
            Telemetry::RestartArdupilot();
            break;
        }
    }
}

float Loop(float dt, float, int, void *) {
    // cap deltatime in case of a long freeze
    dt = std::min(dt, 0.1f);
    if (Calibration::IsEnabled()) {
        Calibration::Loop(dt);
    }
    if (Serial::IsOpen()) {
        Remote::Receive();
        Telemetry::Send();
    } else {
        Serial::Scan();
    }
    return -1.0;
}