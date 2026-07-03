#pragma once

#include "ace/actuator.hpp"

#include <mutex>
#include <string>

namespace ace {

class VirtualActuator : public IActuator {
public:
    VirtualActuator(std::string id, std::string type);

    Clock::time_point apply(const ActuatorCommand& command) override;
    ActuatorState state() const override;

private:
    std::string id_;
    std::string type_;
    mutable std::mutex mutex_;
    ActuatorState state_;
};

} // namespace ace
