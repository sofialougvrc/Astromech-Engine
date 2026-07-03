#pragma once

#include "ace/actuator_bus.hpp"
#include "ace/event_queue.hpp"
#include "ace/sequence.hpp"
#include "ace/sync_engine.hpp"
#include "ace/telemetry.hpp"
#include "ace/trigger.hpp"

#include <atomic>
#include <queue>
#include <vector>

namespace ace {

class Scheduler {
public:
    explicit Scheduler(ActuatorBus& bus);

    void load(const CompiledSequence& sequence);
    void fireTrigger(const Trigger& trigger);
    void run();
    void stop();
    TelemetrySnapshot telemetry() const;

private:
    void dispatchEvent(const TimedEvent& event, Clock::time_point started_at);
    void resetQueue(const std::vector<TimedEvent>& events);

    ActuatorBus& bus_;
    Telemetry telemetry_;
    SyncEngine sync_;
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, TimedEventLater> queue_;
    std::atomic<bool> running_{false};
};

} // namespace ace
