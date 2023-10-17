#include <asio.hpp>
#include <XPLMUtilities.h>
#include <cstdint>
#include <thread>
#include <vector>
#include <string>
#include "serial.hpp"

namespace Serial {
    asio::io_context io_context{};
    asio::serial_port serial{ io_context };
    asio::awaitable<void> Read();
    bool IsOpen() { return serial.is_open(); }
    void PrintError(asio::error_code ec, std::string when);
}

std::vector<std::string> Serial::GetPortsAvailable() {
    std::vector<std::string> ports_available{};
    if (IsOpen()) {
        Disconnect();
    }
    for (int i = 0; i < MAX_SERIAL_PORTS; i++) {
        asio::error_code ec;
        std::string port = std::format("COM{}", i);
        serial.open(port, ec);
        if (ec.value() == 0) {
            try {
                ports_available.push_back(port);
                serial.close();
            }
            catch (asio::system_error e) {
                PrintError(e.code(), "closing a port");
            }
        }
    }
    return ports_available;
}

bool Serial::Connect(std::string port) {
    if (IsOpen()) {
        Disconnect();
    }
    try {
        serial.open(port);
        serial.set_option(asio::serial_port::baud_rate(115200));
        serial.set_option(asio::serial_port::character_size(8));
        serial.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
        EscapeCommFunction(serial.native_handle(), CLRRTS);
        EscapeCommFunction(serial.native_handle(), SETDTR);
        asio::co_spawn(io_context, Read(), asio::detached);
        return true;
    }
    catch (asio::system_error e) {
        PrintError(e.code(), "trying to connect");
        return false;
    }
}

void Serial::Disconnect() {
    if (IsOpen()) {
        asio::error_code ec;
        ::FlushFileBuffers(serial.native_handle());
        serial.close(ec);
        io_context.run();
        if (serial.is_open()) {
            serial = asio::serial_port{ io_context };
        }
    }
}

void Serial::Send(void *buffer, size_t bytes) {
    if (IsOpen()) {
        asio::error_code ec;
        asio::write(serial, asio::buffer(buffer, bytes), ec);
        if (ec.value() != 0) {
            PrintError(ec, "writing to serial port");
            Disconnect();
        }
    }
}

asio::awaitable<void> Serial::Read() {
    char data[1024];
    try {
        std::size_t bytes = co_await asio::async_read(
            serial,
            asio::buffer(data, 1024),
            asio::use_awaitable);
    }
    catch (asio::system_error e) {
        PrintError(e.code(), "reading from serial port");
        Disconnect();
        co_return;
    }
    asio::co_spawn(io_context, Read(), asio::detached);
}

void Serial::Poll() {
    if (IsOpen()) {
        if (io_context.stopped()) {
            io_context.restart();
        }
        ::FlushFileBuffers(serial.native_handle());
        io_context.poll();
    }
}

void Serial::PrintError(asio::error_code ec, std::string when) {
    XPLMDebugString(std::format("Error {}. {}: {}\n",
        when, ec.value(), ec.message()).c_str());
}