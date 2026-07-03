#pragma once

#include "ace/event_queue.hpp"

#include <string>
#include <vector>

namespace ace {

struct CompiledSequence {
    std::string name;
    std::vector<TimedEvent> events;
    int sync_tolerance_ms = 10;
};

CompiledSequence loadSequenceFile(const std::string& path);
CompiledSequence compileImmediateSequence(std::string name, const std::vector<ActuatorCommand>& commands);

} // namespace ace
