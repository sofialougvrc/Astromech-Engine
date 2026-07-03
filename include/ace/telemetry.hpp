#pragma once

#include "ace/actuator.hpp"
#include "ace/sync_engine.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace ace {

struct TelemetrySample {
    std::string actuator;
    int scheduled_ms = 0;
    double actual_ms = 0.0;
    double jitter_ms = 0.0;
};

struct TelemetrySnapshot {
    std::size_t events_dispatched = 0;
    std::size_t deadline_misses = 0;
    double mean_jitter_ms = 0.0;
    double p99_jitter_ms = 0.0;
    double max_jitter_ms = 0.0;
    std::vector<TelemetrySample> samples;
    std::vector<SyncViolation> sync_violations;
};

class Telemetry {
public:
    void clear();
    void record(const ActuatorCommand& command, double actual_ms, double deadline_miss_threshold_ms);
    TelemetrySnapshot snapshot() const;
    void setSyncViolations(std::vector<SyncViolation> violations);

private:
    mutable std::mutex mutex_;
    std::vector<TelemetrySample> samples_;
    std::vector<SyncViolation> sync_violations_;
    std::size_t deadline_misses_ = 0;
};

} // namespace ace
