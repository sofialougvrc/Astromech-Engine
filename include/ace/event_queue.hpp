#pragma once

#include "ace/actuator.hpp"

#include <cstddef>

namespace ace {

struct TimedEvent {
    int t_ms = 0;
    std::size_t order = 0;
    ActuatorCommand command;
};

struct TimedEventLater {
    bool operator()(const TimedEvent& lhs, const TimedEvent& rhs) const {
        if (lhs.t_ms == rhs.t_ms) {
            return lhs.order > rhs.order;
        }
        return lhs.t_ms > rhs.t_ms;
    }
};

} // namespace ace
