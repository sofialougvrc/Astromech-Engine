# Astromech Control Engine (ACE)
### Design notes — real-time brain for the R2 build

---

## 0. Why this design

Real R2 builds run on a Pi-brain / Arduino-actuator split — R2PY puts a Raspberry Pi on top of Arduino boards for the higher-level logic, and MarcDuino uses a Master/Slave Arduino pair where the Master interprets commands from a remote (R2 Touch app) and relays them to the Slave, which drives the actual panels, lights, and sound.

ACE follows that same split, just implemented from scratch in C++ instead of stock MarcDuino firmware, so I own every layer end to end:

| Real R2 role | ACE equivalent |
|---|---|
| Raspberry Pi (R2PY-style brain) | ACE core (scheduler, sequence engine, sync engine) |
| MarcDuino Master (command interpreter) | ActuatorBus / command router |
| MarcDuino Slave (direct actuator I/O) | SerialActuator → Arduino over UART/I2C |
| R2 Touch app (button press → one command) | Trigger Mode |
| N/A — not typical in hobbyist builds | Sequence Mode (scripted, timed, benchmarked cues) |

Most real droids only need Trigger Mode day to day. Sequence Mode is the extra layer I'm adding on top, closer to how stage/theme-park animatronics are cued than to a typical astromech remote — it's what makes the timing/jitter/sync benchmarks meaningful, and it's the part of this that's actually mine rather than a reimplementation of an existing system.

---

## 1. What this is

A C++ real-time scheduler + command router driving multiple actuators (servos, motors, lights, sound), in two modes:
- **Trigger Mode** — single immediate command, mirrors a remote button press
- **Sequence Mode** — timed, multi-track, choreographed cues

Runs on a Raspberry Pi in the final build (same spot R2PY's brain sits), talks to Arduino boards over serial/I2C for actual actuator I/O (same spot MarcDuino's Master/Slave sit). Ships as a library + CLI + dashboard now; becomes the literal controller once hardware exists — same core, swapped actuator backend.

**Design constraint:** the actuator layer has to be swappable (virtual ↔ hardware) without touching the scheduler, command router, or sync engine. Swapping `VirtualActuator` for `SerialActuator` should be a one-line change in the factory, nothing else.

---

## 2. System architecture

```
        ┌───────────────┐        ┌─────────────────────┐
        │ Remote trigger  │        │   Sequence files       │
        │ (single command)│        │   (.seq.json)          │
        └───────┬────────┘        └──────────┬───────────┘
                │                             │ parse
                │                             ▼
                │                  ┌─────────────────────┐
                │                  │   Sequence engine      │
                │                  │   (DSL → event list)    │
                │                  └──────────┬───────────┘
                │                             │
                ▼                             ▼
        ┌────────────────────────────────────────────┐
        │                Core scheduler                  │   ← Pi brain
        │  - monotonic clock, priority queue                │
        │  - dispatch loop (immediate or timed)              │
        │  - jitter / deadline tracking                      │
        └───────┬───────────────────┬──────────────────┘
                │                   │
                ▼                   ▼
     ┌─────────────────┐  ┌──────────────────┐
     │  Actuator bus     │  │   Sync engine      │           ← MarcDuino Master role
     │  (command router)  │  │  (audio ↔ motion)  │
     └───┬─────────┬────┘  └──────────┬─────────┘
         │         │                  │
  ┌──────▼───┐ ┌───▼──────────┐ ┌─────▼────────┐
  │ Virtual   │ │ Serial       │ │ Audio backend │
  │ actuator  │ │ actuator     │ │ (miniaudio)   │
  │ (sim)     │ │ → Arduino(s) │ └───────────────┘
  └──────┬────┘ │ over UART/I2C│                    ← MarcDuino Slave role
         │      └──────────────┘
         ▼
┌──────────────────┐        ┌────────────────────┐
│ Telemetry / metrics │──────▶│  WebSocket bridge    │
└────────────────────┘       └──────────┬───────────┘
                                          ▼
                              ┌───────────────────────┐
                              │  React/D3 dashboard      │
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
│   ├── trigger.hpp             # single-command / remote-trigger path
│   ├── actuator.hpp            # abstract interface
│   ├── actuator_virtual.hpp
│   ├── actuator_serial.hpp     # real HW: Arduino Master/Slave over UART/I2C
│   ├── sync_engine.hpp
│   ├── telemetry.hpp
│   └── ws_bridge.hpp
├── src/
│   └── (mirrors include/, implementations)
├── sequences/
│   ├── dome_spin_beep.seq.json
│   └── examples/...
├── triggers/
│   └── trigger_map.json        # remote button IDs → single commands
├── bench/
│   └── stress_actuators.cpp
├── dashboard/
│   ├── src/RigView.jsx
│   ├── src/TimingGraph.jsx
│   └── src/wsClient.ts
├── tests/
└── docs/
    ├── DSL_SPEC.md
    └── TRIGGER_SPEC.md
```

---

## 4a. Trigger Mode

```json
{
  "trigger_id": "panel_3_open",
  "commands": [
    { "actuator": "dome_panel_3", "type": "servo", "action": "open" },
    { "actuator": "primary_speaker", "type": "audio", "action": "play", "file": "chirp_02.wav" }
  ]
}
```

Dispatched immediately — t=0, no future timestamps. Functionally the same thing MarcDuino does when R2 Touch sends a command byte. Building this first once the core scheduler lands, since it's simpler than sequence mode and it's what actually gets used for live remote-control demos.

## 4b. Sequence Mode

```json
{
  "name": "dome_spin_beep",
  "tracks": [
    {
      "actuator": "dome_rotation",
      "type": "servo",
      "events": [
        { "t_ms": 0, "action": "move_to", "angle_deg": 90, "duration_ms": 800 }
      ]
    },
    {
      "actuator": "front_logic_lights",
      "type": "light",
      "events": [
        { "t_ms": 0, "action": "set", "state": "on" },
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

Both modes route through the same ActuatorBus — a trigger is really just a sequence with one t=0 event per track. Building the scheduler generically (any timed event list) rather than as two separate code paths, since that keeps the two modes consistent for free.

---

## 5. Core scheduler

- **Clock:** `std::chrono::steady_clock` only — never wall clock, not monotonic.
- **Loop model:** sleep until next event's deadline (`sleep_until`) for sequence mode; immediate dispatch for trigger mode.
- **Data structure:** min-heap (`std::priority_queue` or custom binary heap) keyed by `t_ms`.
- **Dispatch:** on wake, pop all events due within a small epsilon window, dispatch to ActuatorBus, record actual-fire-time vs scheduled-time as jitter.
- **Deadline miss policy:** log + increment a counter, don't block waiting on late actuators — one slow channel shouldn't stall everything else.
- **Threading:** single scheduler thread; actuator writes dispatched to a thread pool when the backend blocks (e.g. serial I/O to an Arduino), so a slow motor write can't stall a light cue.

```cpp
class Scheduler {
public:
    void load(const CompiledSequence& seq);
    void fireTrigger(const Trigger& trig);   // immediate-dispatch path
    void run();
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

class VirtualActuator : public IActuator { /* in-memory state, no I/O */ };

// Real hardware: same role as MarcDuino's Master board — takes a high-level
// command and relays it, over UART or I2C, to Arduino "Slave" boards doing
// the direct actuator I/O.
class SerialActuator : public IActuator { /* writes to Arduino Master/Slave over UART/I2C */ };
```

ActuatorBus holds a `map<string, unique_ptr<IActuator>>` built from `actuators.json` at startup. On real hardware this config also specifies which physical Arduino (Master for dome/panels, Slave for lights/sound) each actuator ID lives on.

---

## 7. Sync engine

Audio and motion are fired from the same event queue but have different latency characteristics — audio backend buffering vs. serial write + motor response time. "Synchronized" means bounded drift, not zero drift.

- Each track reports `committed_fire_time` back to the sync engine the moment its backend actually executes the command.
- Drift = `max(committed_fire_time) − min(committed_fire_time)` across tracks scheduled for the same `t_ms`.
- If drift exceeds `sync_tolerance_ms`, log a violation with the offending track IDs.
- v1: measure and report drift honestly, don't try to correct it. Correction is a later feature once real numbers show where drift actually comes from.

---

## 8. Telemetry & benchmarks

- Scheduling jitter: scheduled vs. actual fire time (mean, p99, max)
- Deadline miss rate: % of events fired >X ms late
- Sync drift: cross-track drift as above
- Max concurrent actuators before jitter degrades: ramp actuator count until p99 crosses a defined threshold
- Throughput: events/sec dispatched without missing deadlines

`bench/stress_actuators.cpp` generates synthetic sequences with N virtual actuators on tight overlapping schedules, sweeps N, plots jitter vs N.

---

## 9. Dashboard

- WebSocket bridge (uWebSockets or Boost.Asio) streams ActuatorState at ~60Hz.
- React + D3 rig view — dome/legs/lights as simple shapes, animated live as a sequence plays.
- Second panel: live jitter/drift graph, scrolling window.

---

## 10. Build phases

| Phase | Scope | Est. time |
|---|---|---|
| 1 | Scheduler + priority queue + monotonic clock, unit tests | 3-4 days |
| 2 | Trigger Mode: single-command dispatch, trigger_map.json parsing | 1-2 days |
| 3 | Sequence Mode: DSL parser + compiler, validation | 2-3 days |
| 4 | VirtualActuator + ActuatorBus, wire scheduler → actuators | 2-3 days |
| 5 | Telemetry: jitter/deadline tracking, CLI output | 2 days |
| 6 | Sync engine + drift measurement | 2-3 days |
| 7 | WebSocket bridge + minimal React dashboard | 3-4 days |
| 8 | D3 timing graph, polish rig view | 2-3 days |
| 9 | Benchmark harness + stress test + writeup | 2 days |
| 10 (later) | SerialActuator — real Arduino Master/Slave over UART/I2C | — |

---

## 11. Tech stack

- Core: C++20, CMake, GoogleTest
- Audio: miniaudio or SDL2_mixer
- WebSocket: uWebSockets or Boost.Asio
- Frontend: React + D3.js + Vite
- JSON: nlohmann/json
- CI: GitHub Actions — build + gtest + bench regression check

---

## 12. First step

Scheduler + priority queue + one hardcoded blinking virtual LED, unit test asserting fire time within tolerance. Nothing else until that's solid.
