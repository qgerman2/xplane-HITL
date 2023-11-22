#include <XPLMMenus.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMUtilities.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>
#include <XPWidgetsEx.h>
#include <format>
#include <numeric>
#include "ui.hpp"
#include "main.hpp"
#include "serial.hpp"
#include "calibration.hpp"

namespace UI::Menu {
    XPLMMenuID id;
    void OnEvent(void *mRef, void *iRef);
}

namespace UI::Window {
    XPWidgetID id;
    int width = 200;
    int height = 300;
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
    namespace ButtonDisconnect {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibration {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibrationNextStep {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
    namespace ButtonCalibrationPrevStep {
        XPWidgetID id;
        int OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2);
    }
}

// Menu methods

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
    ListPorts::id = XPCreatePopup(
        20,
        height - 25,
        width - 20,
        height - 40,
        1, "None", id);
    ListPorts::UpdateSerialPorts();
    ButtonConnect::id = XPCreateWidget(
        20,
        height - 60,
        width - 20,
        height - 75,
        1, "Connect", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonConnect::id, ButtonConnect::OnEvent);
    ButtonDisconnect::id = XPCreateWidget(
        20,
        height - 80,
        width - 20,
        height - 95,
        1, "Disconnect", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonDisconnect::id, ButtonDisconnect::OnEvent);
    ButtonCalibration::id = XPCreateWidget(
        20,
        height - 110,
        width - 20,
        height - 125,
        1, "Begin calibration", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibration::id, ButtonCalibration::OnEvent);
    ButtonCalibrationPrevStep::id = XPCreateWidget(
        20,
        height - 130,
        50,
        height - 145,
        1, "<<", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibrationPrevStep::id, ButtonCalibrationPrevStep::OnEvent);
    ButtonCalibrationNextStep::id = XPCreateWidget(
        width - 50,
        height - 130,
        width - 20,
        height - 145,
        1, ">>", 0, id, xpWidgetClass_Button);
    XPAddWidgetCallback(ButtonCalibrationNextStep::id, ButtonCalibrationNextStep::OnEvent);

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
    ports = Serial::GetPortsAvailable();
    if (ports.empty()) {
        ports.push_back("None");
    }
    std::string portstring = std::accumulate(ports.begin() + 1, ports.end(), ports[0],
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
            int port = XPGetWidgetProperty(
                UI::Window::ListPorts::id,
                xpProperty_PopupCurrentItem,
                NULL);
            if (Serial::Connect(UI::Window::ListPorts::ports.at(port))) {
                XPHideWidget(Window::ButtonConnect::id);
            }
        }
        return 1;
    default:
        return 0;
    }
}

int UI::Window::ButtonDisconnect::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        Serial::Disconnect();
        return 1;
    default:
        return 0;
    }
}

void UI::OnSerialDisconnect() {
    XPShowWidget(Window::ButtonConnect::id);
    UI::Window::ListPorts::UpdateSerialPorts();
}

int UI::Window::ButtonCalibration::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        if (Calibration::IsEnabled()) {
            Calibration::Disable();
            XPSetWidgetDescriptor(ButtonCalibration::id, "Begin calibration");
        } else {
            Calibration::Enable();
            XPSetWidgetDescriptor(ButtonCalibration::id, "End calibration");
        }
        return 1;
    default:
        return 0;
    }
}
int UI::Window::ButtonCalibrationNextStep::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        if (Calibration::IsEnabled()) {
            Calibration::NextCalibrationStep();
        }
        return 1;
    default:
        return 0;
    }
}
int UI::Window::ButtonCalibrationPrevStep::OnEvent(XPWidgetMessage inMessage, XPWidgetID inWidget, intptr_t inParam1, intptr_t inParam2) {
    if (inWidget != id) { return 0; }
    switch (inMessage) {
    case xpMsg_PushButtonPressed:
        if (Calibration::IsEnabled()) {
            Calibration::PreviousCalibrationStep();
        }
        return 1;
    default:
        return 0;
    }
}