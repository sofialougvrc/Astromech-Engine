#include "ace/actuator_bus.hpp"
#include "ace/actuator_virtual.hpp"
#include "ace/scheduler.hpp"
#include "ace/sequence.hpp"

#include <iostream>
#include <string>

namespace {

ace::CompiledSequence buildStressSequence(int actuator_count, int events_per_actuator, int spacing_ms) {
    ace::CompiledSequence sequence;
    sequence.name = "stress_" + std::to_string(actuator_count);
    sequence.sync_tolerance_ms = 10;

    std::size_t order = 0;
    for (int actuator = 0; actuator < actuator_count; ++actuator) {
        for (int event_index = 0; event_index < events_per_actuator; ++event_index) {
            ace::ActuatorCommand command;
            command.actuator = "virtual_" + std::to_string(actuator);
            command.type = "light";
            command.action = event_index % 2 == 0 ? "on" : "off";
            command.scheduled_ms = event_index * spacing_ms;
            sequence.events.push_back(ace::TimedEvent{command.scheduled_ms, order++, command});
        }
    }
    return sequence;
}

} // namespace

int main(int argc, char** argv) {
    const int max_actuators = argc > 1 ? std::stoi(argv[1]) : 32;
    const int events_per_actuator = argc > 2 ? std::stoi(argv[2]) : 8;
    const int spacing_ms = argc > 3 ? std::stoi(argv[3]) : 5;

    std::cout << "actuators,events,p99_jitter_ms,max_jitter_ms,deadline_misses\n";
    for (int count = 1; count <= max_actuators; count *= 2) {
        ace::ActuatorBus bus;
        for (int i = 0; i < count; ++i) {
            bus.registerActuator(std::make_unique<ace::VirtualActuator>("virtual_" + std::to_string(i), "light"));
        }

        ace::Scheduler scheduler(bus);
        scheduler.load(buildStressSequence(count, events_per_actuator, spacing_ms));
        scheduler.run();

        const auto telemetry = scheduler.telemetry();
        std::cout << count << ','
                  << telemetry.events_dispatched << ','
                  << telemetry.p99_jitter_ms << ','
                  << telemetry.max_jitter_ms << ','
                  << telemetry.deadline_misses << '\n';
    }
}
