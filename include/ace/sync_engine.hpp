#pragma once

#include "ace/actuator.hpp"

#include <map>
#include <string>
#include <vector>

namespace ace {

struct SyncViolation {
    int scheduled_ms = 0;
    double drift_ms = 0.0;
    std::vector<std::string> actuators;
};

class SyncEngine {
public:
    void reset(int tolerance_ms);
    void record(const ActuatorCommand& command, Clock::time_point committed_at);
    std::vector<SyncViolation> violations() const;

private:
    struct Commit {
        std::string actuator;
        Clock::time_point committed_at;
    };

    int tolerance_ms_ = 10;
    std::map<int, std::vector<Commit>> commits_;
};

} // namespace ace
