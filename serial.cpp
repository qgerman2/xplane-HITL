#include <serialib.h>
#include <XPLMUtilities.h>
#include <cstdint>
#include <thread>
#include <future>
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
    serialib serial;
    int Available() { return serial.available(); };
    bool IsOpen() { return serial.isDeviceOpen(); };
    void Error(std::string what);
    void Disconnect();
    char ping_msg[] = "PINGHITLPINGHITLPING";
    std::stop_source stop_scan;
    std::future<std::optional<std::string>> port_future;
}

void Serial::StopScan() {
    if (!stop_scan.stop_possible()) { return; }
    stop_scan.request_stop();

}

void Serial::Scan() {
    if (!port_future.valid()) {
        stop_scan = std::stop_source{};
        port_future = std::async(std::launch::async,
            []() -> std::optional<std::string> {
                int i = 0;
                while (true) {
                    i++;
                    if (i > MAX_SERIAL_PORTS) { i = 1; }
                    if (stop_scan.stop_requested()) { return std::optional<std::string>(); }
                    // Attempt connection
                    serialib temp_serial;
                    std::string device = std::format("\\\\.\\COM{}", i);
                    if (temp_serial.openDevice(device.c_str(), BAUD_RATE) != 1) { continue; }
                    temp_serial.setDTR();
                    temp_serial.clearRTS();
                    // Scan for header
                    char c;
                    int pos = 0;
                    int bytes_read = 0;
                    while (bytes_read < SCAN_MAXBYTES && temp_serial.readBytes(&c, 1, SCAN_TIMEOUT) == 1) {
                        bytes_read++;
                        if (c == ping_msg[pos]) { pos++; } else { pos = 0; };
                        if (pos == sizeof(ping_msg)) {
                            // Header found, return device name
                            temp_serial.closeDevice();
                            return std::optional<std::string>(device);
                        }
                    }
                }
            });
    }
    if (!port_future.valid()) { return; }
    if (port_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) { return; }
    std::optional<std::string> port = port_future.get();
    if (port.has_value()) {
        serial.openDevice(port.value().c_str(), BAUD_RATE);
        serial.setDTR();
        serial.clearRTS();
        XPLMDebugString(std::format("HITL: Connected to {}\n", port.value()).c_str());
    }
}

void Serial::Disconnect() {
    serial.closeDevice();
    if (IsOpen()) {
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
    if (!IsOpen()) { return false; }
    if (serial.readBytes(dest, 1) <= 0) {
        Error("Failed to read");
        return false;
    }
    return true;
}

void Serial::Error(std::string what) {
    XPLMDebugString(std::format("HITL: Serial error {}.\n", what).c_str());
    Disconnect();
}