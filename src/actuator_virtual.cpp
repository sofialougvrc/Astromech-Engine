#include "ace/actuator_virtual.hpp"

namespace ace {

VirtualActuator::VirtualActuator(std::string id, std::string type)
    : id_(std::move(id)), type_(std::move(type)) {
    state_.actuator = id_;
    state_.type = type_;
    state_.action = "idle";
}

Clock::time_point VirtualActuator::apply(const ActuatorCommand& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.action = command.action;
    state_.params = command.params;
    state_.committed_at = Clock::now();
    return state_.committed_at;
}

ActuatorState VirtualActuator::state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

} // namespace ace
