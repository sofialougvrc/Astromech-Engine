#pragma once

#include "ace/actuator.hpp"

#include <map>
#include <string>
#include <vector>

namespace ace {

struct Trigger {
    std::string trigger_id;
    std::vector<ActuatorCommand> commands;
};

using TriggerMap = std::map<std::string, Trigger>;

TriggerMap loadTriggerMapFile(const std::string& path);

} // namespace ace
