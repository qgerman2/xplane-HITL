#include "XPLMPlugin.h"
#include "XPLMUtilities.h"

PLUGIN_API int XPluginStart(
    char *outName,
    char *outSig,
    char *outDesc) {

    strcpy(outName, "Ardupilot HITL");
    strcpy(outSig, "qgerman2.ardupilot_hitl");
    strcpy(outDesc, "A plugin to enable simple HITL testing with Ardupilot");
    XPLMDebugString("Hola mundo");

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