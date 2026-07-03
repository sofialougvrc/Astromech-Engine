# Astromech Control Engine

ACE is a C++20 prototype control brain for an R2-style astromech build. It follows the Pi-brain / Arduino-actuator split from real R2 builder ecosystems while keeping trigger mode, sequence mode, telemetry, and actuator backends under one swappable core.

## Build

```sh
make all
make test
```

The CMake project is also included for environments with CMake installed.

## Run

```sh
./build/ace_cli sequence sequences/dome_spin_beep.seq.json
./build/ace_cli trigger triggers/trigger_map.json panel_3_open
./build/stress_actuators 8 4 2
```

## Dashboard

```sh
cd dashboard
npm install --cache ../work/npm-cache
npm run dev -- --host 127.0.0.1
```

The dashboard starts with simulated telemetry and will switch to live data when a WebSocket bridge publishes snapshots at `ws://localhost:8765`.

## Current Scope

- Monotonic scheduler with timed event queue.
- Trigger mode compiled through the same path as sequence mode.
- Virtual actuator backend for simulation and tests.
- Serial actuator UART write path for Arduino-style slave boards.
- Jitter, deadline miss, and sync drift telemetry.
- CLI, sample sequences/triggers, benchmark harness, docs, and React/D3 dashboard scaffold.

## Hardware Drafts

- ACE-side serial implementation: `src/actuator_serial.cpp`
- Arduino listener sketch: `firmware/arduino_ace_slave/arduino_ace_slave.ino`
- Example hardware config: `config/actuators.example.json`
- Wire protocol: `docs/SERIAL_PROTOCOL.md`

The serial path is written but not hardware-validated. Start with `PING`, then one LED, then one unloaded servo.
