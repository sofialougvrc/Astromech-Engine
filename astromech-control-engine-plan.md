# Astromech Control Engine (ACE)
### Design notes — full architecture & build plan

---

## 0. Why this design

Real R2 builds run on a Pi-brain / Arduino-actuator split — R2PY puts a Raspberry Pi on top of Arduino boards for the higher-level logic, and MarcDuino uses a Master/Slave Arduino pair where the Master interprets commands from a remote (R2 Touch app) and relays them to the Slave, which drives the actual panels, lights, and sound.

ACE follows that same split, implemented from scratch in C++ instead of stock MarcDuino firmware, so I own every layer end to end:

| Real R2 role | ACE equivalent |
|---|---|
| Raspberry Pi (R2PY-style brain) | ACE core (scheduler, sequence engine, sync engine) |
| MarcDuino Master (command interpreter) | ActuatorBus / command router |
| MarcDuino Slave (direct actuator I/O) | SerialActuator → Arduino over UART/I2C |
| R2 Touch app (button press → one command) | Trigger Mode |
| N/A — not typical in hobbyist builds | Sequence Mode (scripted, timed, benchmarked cues) |

Most real droids only need Trigger Mode day to day. Sequence Mode is the extra layer on top — closer to how stage/theme-park animatronics are cued than to a typical astromech remote — and it's what makes the timing/jitter/sync benchmarks meaningful.

---

## 1. Current status

Software core is complete and verified in simulation:
- Scheduler, both control modes, actuator bus, telemetry, sync engine, CLI, benchmark harness, React/D3 dashboard — all working, tests passing
- Real UART serial code written (`actuator_serial.hpp/.cpp`) — termios config, newline-delimited frames, optional ACK, encodes `SERVO:`/`LIGHT:`/`AUDIO:` commands
- Arduino slave sketch drafted (`arduino_ace_slave.ino`) — parses frames, supports PING/STATUS/SERVO/LIGHT with ACK/error responses
- `actuators.json` example config and `docs/SERIAL_PROTOCOL.md` written

**Not yet done, all hardware-gated:** none of the serial code has touched a real board. First hardware test order: PING → one LED → one unloaded servo, before anything with load or motion range that could bind or strain.

---

## 2. What this is

A C++ real-time scheduler + command router driving multiple actuators (servos, motors, lights, sound), in two modes:
- **Trigger Mode** — single immediate command, mirrors a remote button press
- **Sequence Mode** — timed, multi-track, choreographed cues

Runs on a Raspberry Pi in the final build (same spot R2PY's brain sits), talks to Arduino boards over serial/I2C for actual actuator I/O (same spot MarcDuino's Master/Slave sit).

**Design constraint:** the actuator layer has to be swappable (virtual ↔ hardware) without touching the scheduler, command router, or sync engine.

---

## 3. System architecture

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
         │      └──────┬───────┘
         │             │
         │      ┌──────▼───────────┐
         │      │ Safety layer        │  calibration limits, e-stop,
         │      │ (before any motion)  │  watchdog/reconnect
         │      └──────────────────────┘
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

## 4. Repo layout

```
astromech-control-engine/
├── CMakeLists.txt
├── include/ace/
│   ├── scheduler.hpp
│   ├── event_queue.hpp
│   ├── sequence.hpp
│   ├── trigger.hpp
│   ├── actuator.hpp             # abstract interface
│   ├── actuator_virtual.hpp
│   ├── actuator_serial.hpp      # real HW: Arduino Master/Slave over UART/I2C
│   ├── safety_limits.hpp        # calibration ranges, e-stop, watchdog
│   ├── sync_engine.hpp
│   ├── telemetry.hpp
│   └── ws_bridge.hpp
├── src/
│   └── (mirrors include/, implementations)
├── sequences/
│   ├── dome_spin_beep.seq.json
│   └── examples/...
├── triggers/
│   └── trigger_map.json
├── config/
│   └── actuators.example.json
├── firmware/
│   └── arduino_ace_slave.ino
├── bench/
│   └── stress_actuators.cpp
├── dashboard/
│   ├── src/RigView.jsx
│   ├── src/TimingGraph.jsx
│   └── src/wsClient.ts
├── tests/
└── docs/
    ├── DSL_SPEC.md
    ├── TRIGGER_SPEC.md
    └── SERIAL_PROTOCOL.md
```

---

## 5a. Trigger Mode

```json
{
  "trigger_id": "panel_3_open",
  "commands": [
    { "actuator": "dome_panel_3", "type": "servo", "action": "open" },
    { "actuator": "primary_speaker", "type": "audio", "action": "play", "file": "chirp_02.wav" }
  ]
}
```

Dispatched immediately — t=0, no future timestamps. Functionally the same thing MarcDuino does when R2 Touch sends a command byte.

## 5b. Sequence Mode

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

Both modes route through the same ActuatorBus — a trigger is a sequence with one t=0 event per track.

---

## 6. Core scheduler

- **Clock:** `std::chrono::steady_clock` only — never wall clock.
- **Loop model:** sleep until next event's deadline for sequence mode; immediate dispatch for trigger mode.
- **Data structure:** min-heap keyed by `t_ms`.
- **Dispatch:** on wake, pop all events due within a small epsilon window, dispatch to ActuatorBus, record jitter.
- **Deadline miss policy:** log + increment a counter, don't block waiting on late actuators.
- **Threading:** single scheduler thread; actuator writes dispatched to a thread pool when the backend blocks (serial I/O), so a slow motor write can't stall a light cue.

```cpp
class Scheduler {
public:
    void load(const CompiledSequence& seq);
    void fireTrigger(const Trigger& trig);
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

## 7. Actuator abstraction layer

```cpp
class IActuator {
public:
    virtual void apply(const ActuatorCommand& cmd) = 0;
    virtual ActuatorState state() const = 0;
    virtual ~IActuator() = default;
};

class VirtualActuator : public IActuator { /* in-memory state, no I/O */ };

class SerialActuator : public IActuator {
    // real HW: opens serial device, configures baud via termios,
    // writes newline-delimited frames, optional ACK wait
    // encodes: SERVO:id:angle | SERVO:id:OPEN/CLOSE | LIGHT:pin:ON/OFF | AUDIO:id:PLAY:file
};
```

ActuatorBus holds a `map<string, unique_ptr<IActuator>>` built from `actuators.json`. Config specifies which physical Arduino (Master for dome/panels, Slave for lights/sound) each actuator ID lives on.

---

## 8. Safety layer (built before adding actuators beyond the first)

- **Calibration limits:** min/max angle per servo, enforced in the actuator layer — reject out-of-range commands rather than sending them
- **Hardware dry-run mode:** flag that logs what would be sent without writing to serial
- **Logging to file:** persistent record for debugging after the fact
- **Emergency stop / kill switch:** physical button or serial command that immediately zeroes all actuators — built before anything runs unattended or has multiple motors that could collide
- **Watchdog/reconnect:** detect a dropped Arduino connection and handle it safely rather than silently continuing to issue commands into the void
- **Power-state awareness:** know whether the mount/actuators actually have power before issuing commands

---

## 9. Sync engine

- Each track reports `committed_fire_time` back to the sync engine the moment its backend actually executes the command.
- Drift = `max(committed_fire_time) − min(committed_fire_time)` across tracks scheduled for the same `t_ms`.
- If drift exceeds `sync_tolerance_ms`, log a violation with the offending track IDs.
- v1: measure and report drift honestly, don't correct it yet.

---

## 10. Telemetry & benchmarks

- Scheduling jitter: scheduled vs. actual fire time (mean, p99, max)
- Deadline miss rate
- Sync drift
- Max concurrent actuators before jitter degrades
- Throughput: events/sec dispatched without missing deadlines

`bench/stress_actuators.cpp` sweeps N virtual actuators, plots jitter vs N.

---

## 11. Dashboard

- WebSocket bridge (uWebSockets or Boost.Asio) streams ActuatorState at ~60Hz — currently scaffolded, needs real telemetry wired through once hardware exists
- React + D3 rig view, live jitter/drift graph

---

## 12. Full build phases

| Phase | Scope | Est. time | Status |
|---|---|---|---|
| 1 | Scheduler + priority queue + monotonic clock, unit tests | 3-4 days | Done |
| 2 | Trigger Mode: single-command dispatch | 1-2 days | Done |
| 3 | Sequence Mode: DSL parser + compiler | 2-3 days | Done |
| 4 | VirtualActuator + ActuatorBus | 2-3 days | Done |
| 5 | Telemetry: jitter/deadline tracking | 2 days | Done |
| 6 | Sync engine + drift measurement | 2-3 days | Done |
| 7 | WebSocket bridge + React dashboard | 3-4 days | Done (sim) |
| 8 | Benchmark harness + stress test | 2 days | Done |
| 9 | Real SerialActuator (UART/termios) + Arduino slave sketch | — | Written, untested on hardware |
| 10 | Hardware test: PING → one LED → one unloaded servo | — | Blocked on parts |
| 11 | Safety layer: calibration limits, dry-run mode, logging | — | Blocked on Phase 10 |
| 12 | Emergency stop + watchdog/reconnect | — | Blocked on Phase 10 |
| 13 | Expand actuator zoo: real audio backend, real lights | — | Blocked on Phase 10-12 |
| 14 | Wire real telemetry through WebSocket bridge to dashboard | — | Blocked on Phase 13 |

---

## 13. Tech stack

- Core: C++20, CMake, GoogleTest
- Audio: miniaudio or SDL2_mixer
- WebSocket: uWebSockets or Boost.Asio
- Frontend: React + D3.js + Vite
- JSON: nlohmann/json
- CI: GitHub Actions — build + gtest + bench regression check

---

## 14. Hardware shopping list (Phase 10)

- Arduino Uno or Nano
- One SG90 micro servo
- USB cable
- Small breadboard + jumper wires
- One LED + 220Ω resistor (for the LED test step)
- Separate 5V power supply (not needed for one servo on USB power, but required once more than one actuator is added)

---

## 15. Next concrete step

Order the Phase 10 hardware. Once it arrives: PING first (zero motion, tests the whole serial chain), then the LED (low-stakes wiring check), then the unloaded servo (the real milestone — scheduler-driven motion on real hardware). Nothing in Phase 11 onward starts until Phase 10 passes.
