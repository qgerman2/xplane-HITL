#pragma once
#include <vector>
#include <string>
#include <memory>

#define MAX_SERIAL_PORTS 99
#define BAUD_RATE 115200

struct serial_ports_t {
    std::vector<std::string> names;
    std::vector<std::string> display_names;
};

namespace Serial {
    void Scan();
    void Send(void *buffer, size_t bytes);
    int Available();
    bool Read(uint8_t *dest);
    bool IsOpen();
}