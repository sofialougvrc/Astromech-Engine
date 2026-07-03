#include "ace/telemetry.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ace {

void Telemetry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
    sync_violations_.clear();
    deadline_misses_ = 0;
}

void Telemetry::record(const ActuatorCommand& command, double actual_ms, double deadline_miss_threshold_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    const double jitter = actual_ms - command.scheduled_ms;
    samples_.push_back(TelemetrySample{command.actuator, command.scheduled_ms, actual_ms, jitter});
    if (jitter > deadline_miss_threshold_ms) {
        ++deadline_misses_;
    }
}

TelemetrySnapshot Telemetry::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    TelemetrySnapshot snapshot;
    snapshot.events_dispatched = samples_.size();
    snapshot.deadline_misses = deadline_misses_;
    snapshot.samples = samples_;
    snapshot.sync_violations = sync_violations_;

    if (samples_.empty()) {
        return snapshot;
    }

    std::vector<double> jitters;
    jitters.reserve(samples_.size());
    for (const auto& sample : samples_) {
        jitters.push_back(std::abs(sample.jitter_ms));
    }
    std::sort(jitters.begin(), jitters.end());

    snapshot.mean_jitter_ms = std::accumulate(jitters.begin(), jitters.end(), 0.0) / jitters.size();
    snapshot.max_jitter_ms = jitters.back();
    const auto p99_index = static_cast<std::size_t>(std::ceil(jitters.size() * 0.99)) - 1;
    snapshot.p99_jitter_ms = jitters[std::min(p99_index, jitters.size() - 1)];
    return snapshot;
}

void Telemetry::setSyncViolations(std::vector<SyncViolation> violations) {
    std::lock_guard<std::mutex> lock(mutex_);
    sync_violations_ = std::move(violations);
}

} // namespace ace
