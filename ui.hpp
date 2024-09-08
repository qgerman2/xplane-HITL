#pragma once
#include <string>

namespace UI {
    void OnSerialConnect(std::string port);
    void OnSerialDisconnect();
    namespace Menu {
        void Create();
    }
    namespace Window {
        void Create();
        namespace LabelSerialPort {
            void SetText(std::string text);
        }
        namespace LabelCalibration {
            void SetText(std::string text);
        }
        namespace LabelRemoteArmed {
            void SetText(std::string text);
        }
        namespace LabelAHRSCount {
            void SetText(std::string text);
        }
    }
}