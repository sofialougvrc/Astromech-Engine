#include "ace/scheduler.hpp"

#include <chrono>
#include <thread>

namespace ace {

Scheduler::Scheduler(ActuatorBus& bus) : bus_(bus) {}

void Scheduler::load(const CompiledSequence& sequence) {
    telemetry_.clear();
    sync_.reset(sequence.sync_tolerance_ms);
    resetQueue(sequence.events);
}

void Scheduler::fireTrigger(const Trigger& trigger) {
    load(compileImmediateSequence(trigger.trigger_id, trigger.commands));
    run();
}

void Scheduler::run() {
    running_ = true;
    const auto started_at = Clock::now();

    while (running_ && !queue_.empty()) {
        const TimedEvent next = queue_.top();
        const auto deadline = started_at + std::chrono::milliseconds(next.t_ms);
        std::this_thread::sleep_until(deadline);

        while (!queue_.empty()) {
            const TimedEvent event = queue_.top();
            const auto event_deadline = started_at + std::chrono::milliseconds(event.t_ms);
            if (event_deadline > Clock::now() + std::chrono::milliseconds(1)) {
                break;
            }
            queue_.pop();
            dispatchEvent(event, started_at);
        }
    }

    telemetry_.setSyncViolations(sync_.violations());
    running_ = false;
}

void Scheduler::stop() {
    running_ = false;
}

TelemetrySnapshot Scheduler::telemetry() const {
    return telemetry_.snapshot();
}

void Scheduler::dispatchEvent(const TimedEvent& event, Clock::time_point started_at) {
    const auto committed_at = bus_.dispatch(event.command);
    const auto actual_ms = std::chrono::duration<double, std::milli>(committed_at - started_at).count();
    telemetry_.record(event.command, actual_ms, 10.0);
    sync_.record(event.command, committed_at);
}

void Scheduler::resetQueue(const std::vector<TimedEvent>& events) {
    queue_ = {};
    for (const auto& event : events) {
        queue_.push(event);
    }
}

} // namespace ace
