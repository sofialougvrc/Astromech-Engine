#include "ace/sequence.hpp"

#include "ace/json.hpp"

#include <utility>

namespace ace {

static ActuatorCommand commandFromJson(const Json& source, const std::string& actuator, const std::string& type, int scheduled_ms) {
    ActuatorCommand command;
    command.actuator = actuator.empty() ? source.at("actuator").asString() : actuator;
    command.type = type.empty() ? source.stringOr("type", "generic") : type;
    command.action = source.at("action").asString();
    command.scheduled_ms = scheduled_ms;

    for (const auto& [key, value] : source.asObject()) {
        if (key == "actuator" || key == "type" || key == "action" || key == "t_ms") {
            continue;
        }
        command.params[key] = jsonScalarToString(value);
    }

    return command;
}

CompiledSequence loadSequenceFile(const std::string& path) {
    const Json root = parseJsonFile(path);
    CompiledSequence sequence;
    sequence.name = root.stringOr("name", path);
    sequence.sync_tolerance_ms = root.intOr("sync_tolerance_ms", 10);

    std::size_t order = 0;
    for (const auto& track : root.at("tracks").asArray()) {
        const std::string actuator = track.at("actuator").asString();
        const std::string type = track.stringOr("type", "generic");
        for (const auto& event : track.at("events").asArray()) {
            TimedEvent timed;
            timed.t_ms = event.intOr("t_ms", 0);
            timed.order = order++;
            timed.command = commandFromJson(event, actuator, type, timed.t_ms);
            sequence.events.push_back(std::move(timed));
        }
    }

    return sequence;
}

CompiledSequence compileImmediateSequence(std::string name, const std::vector<ActuatorCommand>& commands) {
    CompiledSequence sequence;
    sequence.name = std::move(name);
    sequence.sync_tolerance_ms = 10;
    sequence.events.reserve(commands.size());
    std::size_t order = 0;
    for (auto command : commands) {
        command.scheduled_ms = 0;
        sequence.events.push_back(TimedEvent{0, order++, std::move(command)});
    }
    return sequence;
}

} // namespace ace
