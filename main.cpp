#include <XPLMPlugin.h>
#include <XPLMMenus.h>
#include <XPLMUtilities.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>

#include "serial.hpp"
#include "ui.hpp"
#include "telemetry.hpp"

float Loop(
    float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop,
    int inCounter, void *inRefcon) {
    if (!Serial::IsOpen()) {
        UI::OnSerialDisconnect();
        return 0.0;
    }
    Telemetry::UpdateState();
    Telemetry::ProcessState();
    Serial::Poll();
    return -1.0;
}

PLUGIN_API int XPluginStart(
    char *outName,
    char *outSig,
    char *outDesc) {

    strcpy(outName, "Ardupilot HITL");
    strcpy(outSig, "https://github.com/qgerman2/xplane-HITL");
    strcpy(outDesc, "A plugin to enable simple HITL testing with Ardupilot");

    UI::Menu::Create();
    UI::Window::Create();

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