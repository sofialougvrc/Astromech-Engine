# Astromech Control Engine (ACE)
### The real-time brain for the R2 build — full architecture & build plan

---

## 1. What this actually is

A C++ real-time scheduler + sequencing engine that drives multiple actuators (servos, motors, lights, sound) in tight synchronization, controlled by a declarative sequence format, with a live React/D3 dashboard for simulation and debugging. Ships as a library + CLI + dashboard now, becomes the literal firmware-adjacent controller for the physical droid later — same core, swapped actuator backend.

**Non-negotiable design constraint:** the actuator layer must be swappable (virtual ↔ hardware) without touching the scheduler, DSL parser, or sync engine. If you can't swap `VirtualActuator` for `SerialActuator` by changing one factory call, the abstraction is wrong.

---

## 2. System architecture

```
                        ┌─────────────────────┐
                        │   Sequence Files      │  (.seq.json)
                        └──────────┬───────────┘
                                   │ parse
                                   ▼
                        ┌─────────────────────┐
                        │   Sequence Engine     │  validates, compiles
                        │   (DSL → event list)  │  to timed EventQueue
                        └──────────┬───────────┘
                                   │
                                   ▼
              ┌────────────────────────────────────────┐
              │              Core Scheduler               │
              │  - monotonic clock, priority queue          │
              │  - dispatch loop (fixed tick or event-driven)│
              │  - jitter/deadline tracking                  │
              └───────┬───────────────────┬──────────────┘
                      │                   │
                      ▼                   ▼
           ┌─────────────────┐  ┌──────────────────┐
           │  Actuator Bus     │  │   Sync Engine      │
           │  (interface layer)│  │  (audio ↔ motion)  │
           └───┬─────────┬────┘  └──────────┬─────────┘
               │         │                  │
        ┌──────▼───┐ ┌───▼──────┐    ┌──────▼───────┐
        │ Virtual   │ │ Serial   │    │ Audio backend │
        │ Actuator  │ │ Actuator │    │ (miniaudio)   │
        │ (sim)     │ │ (real HW)│    └───────────────┘
        └──────┬────┘ └──────────┘
               │
               ▼
      ┌──────────────────┐        ┌────────────────────┐
      │ Telemetry/Metrics  │──────▶│  WebSocket Bridge    │
      │ (jitter, drift,    │       │  (state @ 60Hz)      │
      │  deadline misses)  │       └──────────┬───────────┘
      └────────────────────┘                  │
                                               ▼
                                   ┌───────────────────────┐
                                   │  React/D3 Dashboard      │
                                   │  live rig view + graphs  │
                                   └───────────────────────┘
```

---

## 3. Repo layout

```
astromech-control-engine/
├── CMakeLists.txt
├── include/ace/
│   ├── scheduler.hpp
│   ├── event_queue.hpp
│   ├── sequence.hpp
│   ├── actuator.hpp            # abstract interface
│   ├── actuator_virtual.hpp
│   ├── actuator_serial.hpp     # real HW, stub until droid exists
│   ├── sync_engine.hpp
│   ├── telemetry.hpp
│   └── ws_bridge.hpp
├── src/
│   └── (mirrors include/, implementations)
├── sequences/
│   ├── dome_spin_beep.seq.json
│   └── examples/...
├── bench/
│   └── stress_actuators.cpp    # benchmark harness
├── dashboard/                  # React + D3 + Vite
│   ├── src/RigView.jsx
│   ├── src/TimingGraph.jsx
│   └── src/wsClient.ts
├── tests/
│   └── (gtest: scheduler, parser, sync tolerance)
└── docs/
    └── DSL_SPEC.md
```

---

## 4. Sequence DSL

Declarative, JSON-based (swap for a custom text DSL later if you want a real parser-design exercise — recommend starting JSON so you're not fighting two hard problems at once).

```json
{
  "name": "dome_spin_beep",
  "tracks": [
    {
      "actuator": "dome_rotation",
      "type": "servo",
      "events": [
        { "t_ms": 0,    "action": "move_to", "angle_deg": 90, "duration_ms": 800 }
      ]
    },
    {
      "actuator": "front_logic_lights",
      "type": "light",
      "events": [
        { "t_ms": 0,   "action": "set", "state": "on" },
        { "t_ms": 800, "action": "set", "state": "off" }
      ]
    },
    {
      "actuator": "primary_speaker",
      "type": "audio",
      "events": [
        { "t_ms": 50, "action": "play", "file": "beep_03.wav" }
      ]
    }
  ],
  "sync_tolerance_ms": 10
}
```

Compilation step: sequence engine flattens all tracks into one global `EventQueue` sorted by `t_ms`, resolves actuator IDs against the registered actuator bus, and rejects the sequence at load time if any actuator ID is unknown or any timing is negative/overlapping in a way the actuator can't physically do (e.g. two conflicting `move_to` on the same servo at the same timestamp).

---

## 5. Core scheduler design

- **Clock:** `std::chrono::steady_clock` only — never wall clock, it's not monotonic.
- **Loop model:** hybrid. Sleep until next event's deadline (`sleep_until`), not a busy-poll tick — lower CPU, and jitter is dominated by OS scheduling, not your loop.
- **Data structure:** min-heap (`std::priority_queue` or a custom binary heap) keyed by `t_ms`, so next-event lookup is O(1) and insertion is O(log n).
- **Dispatch:** on wake, pop all events due within a small epsilon window, dispatch to `ActuatorBus`, record actual-fire-time vs scheduled-time as jitter.
- **Deadline miss policy:** log + increment a counter, do NOT block waiting for late actuators — animatronics has to keep moving even if one channel lags, or it looks broken.
- **Threading:** single scheduler thread; actuator writes dispatched to a thread pool if a given actuator backend is blocking (e.g. serial I/O), so a slow motor write can't stall the dome light.

Interface sketch:

```cpp
class Scheduler {
public:
    void load(const CompiledSequence& seq);
    void run();                    // blocks until sequence complete
    void stop();
    TelemetrySnapshot telemetry() const;
private:
    std::priority_queue<TimedEvent> queue_;
    ActuatorBus& bus_;
    Telemetry telemetry_;
};
```

---

## 6. Actuator abstraction layer

```cpp
class IActuator {
public:
    virtual void apply(const ActuatorCommand& cmd) = 0;
    virtual ActuatorState state() const = 0;
    virtual ~IActuator() = default;
};

class VirtualActuator : public IActuator { /* updates in-memory state, no I/O */ };
class SerialActuator  : public IActuator { /* writes to Arduino/RPi over UART */ };
```

`ActuatorBus` holds a `map<string, unique_ptr<IActuator>>` built from a config file (`actuators.json`) at startup — this is the one place that knows whether you're running in sim or on real hardware. Everything upstream (scheduler, DSL, sync engine) never knows the difference. This is the seam that makes today's software directly reusable on the physical droid.

---

## 7. Sync engine (the actually hard part)

The core problem: audio playback and motor movement are fired from the same event queue but have wildly different latency characteristics (audio backend buffering vs. serial write + motor response time). "Synchronized" means bounded drift, not zero drift.

- Each track reports a `committed_fire_time` back to the sync engine the moment its backend actually executes the command (not when it was scheduled).
- Sync engine computes drift = `max(committed_fire_time) - min(committed_fire_time)` across tracks that were scheduled for the same `t_ms`.
- If drift exceeds `sync_tolerance_ms` from the sequence file, log a sync violation with the offending track IDs — this becomes one of your headline benchmark numbers.
- For v1, don't try to *correct* drift (e.g. resampling audio) — just measure and report it honestly. Correction is a legitimate v2 feature once you have real numbers showing where drift comes from.

---

## 8. Telemetry & benchmark harness

Metrics to capture, per sequence run:
- **Scheduling jitter:** scheduled vs. actual fire time per event (mean, p99, max), in µs/ms
- **Deadline miss rate:** % of events fired >X ms late
- **Sync drift:** cross-track drift as defined above
- **Max concurrent actuators before jitter degrades:** stress test — ramp actuator count until p99 jitter crosses a threshold you define upfront (e.g. 5ms). This is your FluxCore/SpecDec-style headline number.
- **Throughput:** events/sec the scheduler can dispatch without missing deadlines

`bench/stress_actuators.cpp` — generate synthetic sequences with N virtual actuators firing on tight overlapping schedules, sweep N, plot jitter vs N. This is the number you put on the CV: *"Real-time scheduler sustains sub-Xms p99 jitter across N simulated actuators."*

---

## 9. Dashboard

- WebSocket bridge (simple, e.g. `uWebSockets` or even a small `Boost.Asio` server) streams `ActuatorState` snapshots at ~60Hz to the frontend.
- React + D3: a simple top-down/front rig view (circles/rects for dome, legs, lights — doesn't need to be a 3D model for v1) that animates live as a sequence plays.
- Second panel: real-time jitter/drift graph (D3 line chart), scrolling window, so you can *see* timing problems as they happen rather than only in post-run logs.
- This is your demo artifact — a video of a virtual droid executing a synchronized light/sound/motion sequence with a live timing graph next to it is a genuinely strong portfolio piece even before any hardware exists.

---

## 10. Build phases

| Phase | Scope | Est. time |
|---|---|---|
| 1 | Scheduler + priority queue + monotonic clock, unit tests, no actuators yet | 3-4 days |
| 2 | Sequence DSL parser + compiler (JSON → EventQueue), validation | 2-3 days |
| 3 | VirtualActuator + ActuatorBus, wire scheduler → actuators end to end | 2-3 days |
| 4 | Telemetry: jitter/deadline tracking, CLI text output | 2 days |
| 5 | Sync engine + drift measurement | 2-3 days |
| 6 | WebSocket bridge + minimal React dashboard (state view only) | 3-4 days |
| 7 | D3 timing graph, polish rig view | 2-3 days |
| 8 | Benchmark harness + stress test + writeup with real numbers | 2 days |
| 9 (later) | `SerialActuator` implementation once hardware exists | — |

Total for a demoable v1 with real benchmarks: **~3 weeks** at a steady pace, consistent with your other project timelines.

---

## 11. Tech stack

- **Core:** C++20, CMake, GoogleTest
- **Audio:** miniaudio (single-header, easy to embed) or SDL2_mixer
- **WebSocket:** uWebSockets or Boost.Asio
- **Frontend:** React + D3.js + Vite (matches FluxCore/DriftGuard stack)
- **JSON:** nlohmann/json for sequence parsing
- **CI:** GitHub Actions — build + gtest + bench regression check (this pairs naturally with your DriftGuard tool: run the stress benchmark on every commit and flag jitter regressions the same way you already flag perf regressions)

---

## 12. First concrete step

Start with Phase 1 only: scheduler + priority queue + one hardcoded event ("blink a virtual LED at t=0, t=500, t=1000") with a unit test asserting fire time is within Xµs of scheduled time. Nothing else — no DSL, no actuators plural, no dashboard — until that's solid and tested. Everything else builds on this being correct.
