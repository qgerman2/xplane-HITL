#pragma once
#include <vector>
#include <string>
#include <memory>

#define MAX_SERIAL_PORTS 256

namespace Serial {
    std::vector<std::string> GetPortsAvailable();
    bool Connect(std::string port);
    void Disconnect();
    void Send(void *buffer, size_t bytes);
    void Poll();
    bool IsOpen();
}