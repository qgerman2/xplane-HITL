#include <XPLMMenus.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMUtilities.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>
#include <XPWidgetsEx.h>
#include <format>
#include <numeric>
#include <string>
#include "ui.hpp"
#include "main.hpp"
#include "serial.hpp"
#include "calibration.hpp"
#include "remote.hpp"

// X-Plane top menu plugin definitions

namespace UI::Menu {
    XPLMMenuID id;
    void OnEvent(void *mRef, void *iRef);
}

// Main window definitions

namespace UI::Window {
    XPWidgetID id;
    int width = 260;
    int height = 125;
    int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    namespace ListPorts {
        XPWidgetID id;
        std::vector<std::string> ports;
        void UpdateSerialPorts();
    }
    namespace ButtonConnect {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonSerialScan {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibration {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace LabelCalibration {
        XPWidgetID id;
    }
    namespace ButtonCalibrationNextStep {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibrationPrevStep {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibrationCompass {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonRemoteOverride {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace LabelRemoteArmed {
        XPWidgetID id;
    }
}

// Top menu methods

void UI::Menu::Create() {
    int index = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "HITL", NULL, 1);
    id = XPLMCreateMenu("HITL", XPLMFindPluginsMenu(), index, OnEvent, NULL);
    XPLMAppendMenuItem(id, "Settings", (void *)"Settings", 1);
}

void UI::Menu::OnEvent(void *mRef, void *iRef) {
    if (!Serial::IsOpen()) {
        UI::Window::ListPorts::UpdateSerialPorts();
    }
    XPShowWidget(UI::Window::id);
}

// Main window methods

void UI::Window::Create() {
    id = XPCreateWidget(0, height, width, 0, 1, "HITL Settings", 1, 0, xpWidgetClass_MainWindow);
    //XPHideWidget(id);
    XPSetWidgetProperty(id, xpProperty_MainWindowHasCloseBoxes, 1);
    XPAddWidgetCallback(id, OnEvent);
    // -- Serial Widgets --
    XPCreateWidget(
        10 - 2,
        height - 20 - 2,
        80,
        height - 35,
        1, "Serial", 0, id, xpWidgetClass_Caption);
    ListPorts::id = XPCreatePopup(
        10 + 4,
        height - 40 + 1,
        80 - 4,
        height - 55 + 1,
        1, "None", id);
    ListPorts::UpdateSerialPorts();
    ButtonConnect::id = XPCreateWidget(
        10,
        height - 60,
        80,
        height - 75,
        1, "Connect", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonConnect::id, ButtonConnect::OnEvent);
    ButtonSerialScan::id = XPCreateWidget(
        10,
        height - 80,
        80,
        height - 95,
        1, "Refresh", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonSerialScan::id, ButtonSerialScan::OnEvent);
    // -- Calibration Widgets --
    XPCreateWidget(
        95 - 2,
        height - 20 - 2,
        165,
        height - 35,
        1, "Calibration", 0, id, xpWidgetClass_Caption);
    ButtonCalibration::id = XPCreateWidget(
        95,
        height - 40,
        165,
        height - 55,
        1, "Begin", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibration::id, ButtonCalibration::OnEvent);
    LabelCalibration::id = XPCreateWidget(
        95,
        height - 60 + 3,
        165,
        height - 75,
        1, "None", 0, id, xpWidgetClass_Caption);
    ButtonCalibrationPrevStep::id = XPCreateWidget(
        95,
        height - 80,
        95 + 30,
        height - 95,
        1, "<<", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibrationPrevStep::id, ButtonCalibrationPrevStep::OnEvent);
    ButtonCalibrationNextStep::id = XPCreateWidget(
        165 - 30,
        height - 80,
        165,
        height - 95,
        1, ">>", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibrationNextStep::id, ButtonCalibrationNextStep::OnEvent);
    ButtonCalibrationCompass::id = XPCreateWidget(
        95,
        height - 100,
        165,
        height - 115,
        1, "Rotate", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibrationCompass::id, ButtonCalibrationCompass::OnEvent);
    // -- Remote control Widgets --
    XPCreateWidget(
        180 - 2,
        height - 20 - 2,
        250,
        height - 35,
        1, "Remote", 0, id, xpWidgetClass_Caption);
    ButtonRemoteOverride::id = XPCreateWidget(
        180,
        height - 40,
        195,
        height - 55,
        1, "", 0, id, xpWidgetClass_Button);
    XPCreateWidget(
        195 - 2,
        height - 40 + 3,
        250,
        height - 55,
        1, "Override", 0, id, xpWidgetClass_Caption);
    LabelRemoteArmed::id = XPCreateWidget(
        180 - 2,
        height - 60 + 3,
        250,
        height - 75,
        1, "None", 0, id, xpWidgetClass_Caption);
    XPSetWidgetProperty(ButtonRemoteOverride::id, xpProperty_ButtonType, xpRadioButton);
    XPSetWidgetProperty(ButtonRemoteOverride::id, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
    XPSetWidgetProperty(ButtonRemoteOverride::id, xpProperty_ButtonState, true);
    XPAddWidgetCallback(ButtonRemoteOverride::id, ButtonRemoteOverride::OnEvent);

    int screenWidth;
    int screenHeight;
    XPLMGetScreenSize(&screenWidth, &screenHeight);
    XPSetWidgetGeometry(id,
        screenWidth / 2 - width / 2,
        screenHeight / 2 + height / 2,
        screenWidth / 2 + width / 2,
        screenHeight / 2 - height / 2);
}

int UI::Window::OnEvent(
    XPWidgetMessage inMessage,
    XPWidgetID inWidget,
    intptr_t inParam1,
    intptr_t inParam2) {
    if (inMessage == xpMessage_CloseButtonPushed) {
        XPHideWidget(id);
        return 1;
    }
    return 0;
}

void UI::Window::ListPorts::UpdateSerialPorts() {
    serial_ports_t serial_ports = Serial::GetPortsAvailable();
    if (serial_ports.names.empty()) {
        serial_ports.names.push_back("None");
        serial_ports.display_names.push_back("None");
    }
    ListPorts::ports = std::move(serial_ports.names);
    std::string portstring = std::accumulate(
        serial_ports.display_names.begin() + 1,
        serial_ports.display_names.end(),
        serial_ports.display_names[0],
        [](std::string x, std::string y) {
            return x + ";" + y;
        });
    XPSetWidgetProperty(UI::Window::ListPorts::id, xpProperty_PopupCurrentItem, 0);
    XPSetWidgetDescriptor(UI::Window::ListPorts::id, portstring.c_str());
}

int UI::Window::ButtonConnect::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        if (!Serial::IsOpen()) {
            int port = XPGetWidgetProperty(ListPorts::id, xpProperty_PopupCurrentItem, NULL);
            Serial::Connect(ListPorts::ports.at(port));
        } else {
            Serial::Disconnect();
        }
        return 1;
    default:
        return 0;
    }
}

void UI::OnSerialConnect() {
    XPSetWidgetDescriptor(Window::ButtonConnect::id, "Disconnect");
}

void UI::OnSerialDisconnect() {
    XPSetWidgetDescriptor(Window::ButtonConnect::id, "Connect");
    UI::Window::ListPorts::UpdateSerialPorts();
    UI::Window::LabelRemoteArmed::SetText("None");
}

int UI::Window::ButtonSerialScan::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        ListPorts::UpdateSerialPorts();
        return 1;
    default:
        return 0;
    }
}

int UI::Window::ButtonCalibration::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        if (Calibration::Toggle()) {
            XPSetWidgetDescriptor(id, "End");
        } else {
            XPSetWidgetDescriptor(id, "Begin");
        }
        return 1;
    default:
        return 0;
    }
}
void UI::Window::LabelCalibration::SetText(std::string text) {
    XPSetWidgetDescriptor(id, text.c_str());
}
int UI::Window::ButtonCalibrationNextStep::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        Calibration::NextCalibrationStep();
        return 1;
    default:
        return 0;
    }
}
int UI::Window::ButtonCalibrationPrevStep::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        Calibration::PreviousCalibrationStep();
        return 1;
    default:
        return 0;
    }
}
int UI::Window::ButtonCalibrationCompass::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        Calibration::ToggleRotation();
        return 1;
    default:
        return 0;
    }
}
int UI::Window::ButtonRemoteOverride::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_ButtonStateChanged:
        Remote::SetOverride(inParam2);
        return 1;
    default:
        return 0;
    }
}
void UI::Window::LabelRemoteArmed::SetText(std::string text) {
    XPSetWidgetDescriptor(id, text.c_str());
}