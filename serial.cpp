#include <serialib.h>
#include <XPLMUtilities.h>
#include <cstdint>
#include <thread>
#include <vector>
#include <string>
#include <format>
#include "serial.hpp"

namespace Serial {
    serialib serial;
    void Read();
    bool IsOpen() { return serial.isDeviceOpen(); };
    void PrintError(int code, std::string what);
}

std::vector<std::string> Serial::GetPortsAvailable() {
    if (IsOpen()) {
        Disconnect();
    }
    std::vector<std::string> ports_available{};
    for (int i = 1; i < MAX_SERIAL_PORTS; i++) {
        char device_name[64];
        sprintf(device_name, "\\\\.\\COM%d", i);
        XPLMDebugString(device_name);
        if (serial.openDevice(device_name, BAUD_RATE) == 1) {
            serial.closeDevice();
            ports_available.push_back(std::string(device_name));
        }
    }
    return ports_available;
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
        return true;
    } else {
        PrintError(res, "trying to connect");
        return false;
    }
}

void Serial::Disconnect() {
    if (IsOpen()) {
        serial.closeDevice();
    }
}

void Serial::Send(void *buffer, size_t bytes) {
    if (IsOpen()) {
        serial.writeBytes(buffer, bytes);
    }
}

void Serial::Read() {
    char data[1024];
    int pos = 0;
    int bytes = serial.available();
    serial.readBytes(data, 1024);
}

void Serial::Poll() {
    if (IsOpen()) {
        //Read();
    }
}

void Serial::PrintError(int code, std::string what) {
    XPLMDebugString(std::format("Error {}. [{}]\n",
        what, code).c_str());
}