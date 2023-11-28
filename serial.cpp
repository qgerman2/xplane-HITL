#include <serialib.h>
#include <XPLMUtilities.h>
#include <cstdint>
#include <thread>
#include <vector>
#include <string>
#include <format>
#include "main.hpp"
#include "serial.hpp"
#include "ui.hpp"
#include "remote.hpp"

namespace Serial {
    serialib serial;
    int Available() { return serial.available(); };
    bool IsOpen() { return serial.isDeviceOpen(); };
    void Error(std::string what);
}

serial_ports_t Serial::GetPortsAvailable() {
    if (IsOpen()) {
        Disconnect();
    }
    serial_ports_t serial_ports{};
    for (int i = 1; i < MAX_SERIAL_PORTS; i++) {
        char device_name[64];
        sprintf(device_name, "\\\\.\\COM%d", i);
        if (serial.openDevice(device_name, BAUD_RATE) == 1) {
            serial.closeDevice();
            serial_ports.names.emplace_back(std::string(device_name));
            serial_ports.display_names.emplace_back(std::format({ "COM {}" }, i));
        }
    }
    return serial_ports;
}

bool Serial::Connect(std::string port) {
    if (IsOpen()) {
        Disconnect();
    }
    // baud 115200, 8N1
    int res = serial.openDevice(port.c_str(), 115200);
    if (res == 1) {
        serial.setDTR();
        serial.clearRTS();
        Remote::Enable();
        return true;
    } else {
        return false;
    }
}

void Serial::Disconnect() {
    if (IsOpen()) {
        serial.closeDevice();
        UI::OnSerialDisconnect();
        Remote::Disable();
    }
}

void Serial::Send(void *buffer, size_t bytes) {
    if (IsOpen()) {
        if (serial.writeBytes(buffer, bytes) == -1) {
            Error("Failed to write");
        }
    }
}

bool Serial::Read(uint8_t *dest) {
    if (serial.readBytes(dest, 1) >= 0) {
        return true;
    } else {
        Error("Failed to read");
        return false;
    }
}

void Serial::Error(std::string what) {
    XPLMDebugString(std::format("Serial error: {}.\n", what).c_str());
    Disconnect();
}