#pragma once

#include "ace/actuator.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ace {

class ActuatorBus {
public:
    void registerActuator(std::unique_ptr<IActuator> actuator);
    bool hasActuator(const std::string& actuator_id) const;
    Clock::time_point dispatch(const ActuatorCommand& command);
    std::vector<ActuatorState> states() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<IActuator>> actuators_;
};

} // namespace ace
