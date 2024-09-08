#pragma once
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <stop_token>

#define MAX_SERIAL_PORTS 99
#define BAUD_RATE 115200

struct serial_ports_t {
    std::vector<std::string> names;
    std::vector<std::string> display_names;
};

namespace Serial {
    void Send(void *buffer, size_t bytes);
    int Available();
    void Disconnect();
    bool Read(uint8_t *dest);
    bool IsOpen();
    void Scan();
    void StopScan();
}