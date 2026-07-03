#include "ace/trigger.hpp"

#include "ace/json.hpp"

namespace ace {

TriggerMap loadTriggerMapFile(const std::string& path) {
    const Json root = parseJsonFile(path);
    TriggerMap triggers;

    for (const auto& trigger_json : root.at("triggers").asArray()) {
        Trigger trigger;
        trigger.trigger_id = trigger_json.at("trigger_id").asString();
        for (const auto& command_json : trigger_json.at("commands").asArray()) {
            ActuatorCommand command;
            command.actuator = command_json.at("actuator").asString();
            command.type = command_json.stringOr("type", "generic");
            command.action = command_json.at("action").asString();
            command.scheduled_ms = 0;
            for (const auto& [key, value] : command_json.asObject()) {
                if (key == "actuator" || key == "type" || key == "action") {
                    continue;
                }
                command.params[key] = jsonScalarToString(value);
            }
            trigger.commands.push_back(std::move(command));
        }
        triggers[trigger.trigger_id] = std::move(trigger);
    }

    return triggers;
}

} // namespace ace
