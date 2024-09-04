#include <serialib.h>
#include <XPLMUtilities.h>
#include <cstdint>
#include <thread>
#include <vector>
#include <string>
#include <format>
#include <thread>
#include <optional>
#include "main.hpp"
#include "serial.hpp"
#include "ui.hpp"
#include "remote.hpp"
#include "telemetry.hpp"

#define SCAN_TIMEOUT 100
#define SCAN_MAXBYTES 200

namespace Serial {
    std::optional<serialib> serial;
    int Available() { return serial.has_value() ? serial->available() : 0; };
    bool IsOpen() { return serial.has_value() ? serial->isDeviceOpen() : false; };
    void Error(std::string what);
    void Disconnect();
    std::thread t;
}

void Serial::Scan() {
    t = std::thread([&]() -> void {
        while (true) {
            if (!serial.has_value()) {
                for (int i = 1; i < MAX_SERIAL_PORTS; i++) {
                    // Attempt connection
                    serialib temp_serial;
                    std::string device = std::format("COM{}", i);
                    if (temp_serial.openDevice(device.c_str(), BAUD_RATE) != 1) { continue; }
                    temp_serial.setDTR();
                    temp_serial.clearRTS();
                    // Scan for header
                    char c;
                    int pos = 0;
                    int bytes_read = 0;
                    while (bytes_read < SCAN_MAXBYTES && temp_serial.readBytes(&c, 1, SCAN_TIMEOUT) == 1) {
                        bytes_read++;
                        if (c == header[pos]) { pos++; }
                        if (pos == sizeof(header)) {
                            // Header found, pass serial obj to main thread
                            serial.emplace(std::move(temp_serial));
                            Remote::Enable();
                            Telemetry::Reset();
                            UI::OnSerialConnect();
                            break;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        });
}

void Serial::Disconnect() {
    if (IsOpen()) {
        serial->closeDevice();
        UI::OnSerialDisconnect();
        Remote::Disable();
    }
    if (serial.has_value()) {
        serial.reset();
    }
}

void Serial::Send(void *buffer, size_t bytes) {
    if (IsOpen()) {
        if (serial->writeBytes(buffer, bytes) == -1) {
            Error("Failed to write");
        }
    }
}

bool Serial::Read(uint8_t *dest) {
    if (IsOpen() && serial->readBytes(dest, 1) >= 0) {
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