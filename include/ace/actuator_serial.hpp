#pragma once

#include "ace/actuator.hpp"

#include <mutex>
#include <optional>
#include <string>

namespace ace {

struct SerialEndpoint {
    std::string device;
    int baud_rate = 115200;
    std::string channel = "1";
    bool wait_for_ack = false;
    int ack_timeout_ms = 50;
};

class SerialActuator : public IActuator {
public:
    SerialActuator(std::string id, std::string type, SerialEndpoint endpoint);
    ~SerialActuator() override;

    Clock::time_point apply(const ActuatorCommand& command) override;
    ActuatorState state() const override;

private:
    void ensureOpen();
    void closePort();
    void writeFrame(const std::string& frame);
    std::optional<std::string> readAck();

    std::string id_;
    std::string type_;
    SerialEndpoint endpoint_;
    int fd_ = -1;
    mutable std::mutex mutex_;
    ActuatorState state_;
};

std::string encodeSerialFrame(const ActuatorCommand& command, const SerialEndpoint& endpoint);

} // namespace ace
