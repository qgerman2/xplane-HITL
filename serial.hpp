#pragma once
#include <vector>
#include <string>
#include <memory>

#define MAX_SERIAL_PORTS 99
#define BAUD_RATE 115200

namespace Serial {
    std::vector<std::string> GetPortsAvailable();
    bool Connect(std::string port);
    void Disconnect();
    void Send(void *buffer, size_t bytes);
    int Available();
    bool Read(uint8_t *dest);
    bool IsOpen();
}