#pragma once

#include <chrono>
#include <map>
#include <string>

namespace ace {

using Clock = std::chrono::steady_clock;

struct ActuatorCommand {
    std::string actuator;
    std::string type;
    std::string action;
    std::map<std::string, std::string> params;
    int scheduled_ms = 0;
};

struct ActuatorState {
    std::string actuator;
    std::string type;
    std::string action;
    std::map<std::string, std::string> params;
    Clock::time_point committed_at{};
};

class IActuator {
public:
    virtual ~IActuator() = default;
    virtual Clock::time_point apply(const ActuatorCommand& command) = 0;
    virtual ActuatorState state() const = 0;
};

} // namespace ace
