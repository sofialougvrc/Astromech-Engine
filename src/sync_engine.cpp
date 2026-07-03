#include "ace/sync_engine.hpp"

#include <algorithm>
#include <chrono>

namespace ace {

void SyncEngine::reset(int tolerance_ms) {
    tolerance_ms_ = tolerance_ms;
    commits_.clear();
}

void SyncEngine::record(const ActuatorCommand& command, Clock::time_point committed_at) {
    commits_[command.scheduled_ms].push_back(Commit{command.actuator, committed_at});
}

std::vector<SyncViolation> SyncEngine::violations() const {
    std::vector<SyncViolation> result;
    for (const auto& [scheduled_ms, commits] : commits_) {
        if (commits.size() < 2) {
            continue;
        }

        auto minmax = std::minmax_element(
            commits.begin(),
            commits.end(),
            [](const Commit& lhs, const Commit& rhs) {
                return lhs.committed_at < rhs.committed_at;
            });
        const auto drift = std::chrono::duration<double, std::milli>(
            minmax.second->committed_at - minmax.first->committed_at).count();
        if (drift <= tolerance_ms_) {
            continue;
        }

        SyncViolation violation;
        violation.scheduled_ms = scheduled_ms;
        violation.drift_ms = drift;
        for (const auto& commit : commits) {
            violation.actuators.push_back(commit.actuator);
        }
        result.push_back(std::move(violation));
    }
    return result;
}

} // namespace ace
