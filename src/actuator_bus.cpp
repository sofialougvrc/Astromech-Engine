#include "ace/actuator_bus.hpp"

#include <stdexcept>

namespace ace {

void ActuatorBus::registerActuator(std::unique_ptr<IActuator> actuator) {
    const auto id = actuator->state().actuator;
    std::lock_guard<std::mutex> lock(mutex_);
    actuators_[id] = std::move(actuator);
}

bool ActuatorBus::hasActuator(const std::string& actuator_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return actuators_.contains(actuator_id);
}

Clock::time_point ActuatorBus::dispatch(const ActuatorCommand& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto found = actuators_.find(command.actuator);
    if (found == actuators_.end()) {
        throw std::runtime_error("unknown actuator: " + command.actuator);
    }
    return found->second->apply(command);
}

std::vector<ActuatorState> ActuatorBus::states() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ActuatorState> result;
    result.reserve(actuators_.size());
    for (const auto& [_, actuator] : actuators_) {
        result.push_back(actuator->state());
    }
    return result;
}

} // namespace ace
