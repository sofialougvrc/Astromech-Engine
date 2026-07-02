# Astromech-Engine

Real-time control engine for synchronized actuator sequencing — the software
brain for an astromech droid build. Coordinates motors, lights, and audio
against a declarative timed sequence, with a swappable actuator backend
(simulated now, real hardware later).

## Status
Early development. Currently building the core scheduler.

## Why
Animatronics' hard problem isn't moving one motor — it's keeping many
actuators in sync under real-time constraints. This is that problem, solved
in software first, in simulation, before any hardware exists.

## Architecture
See https://github.com/sofiagvrc/Astromech-Engine/blob/master/docs/PLAN.md for full design (scheduler, sequence DSL,
sync engine, benchmark plan).

## Roadmap
- [ ] Core scheduler (priority queue, monotonic clock, jitter tracking)
- [ ] Sequence DSL parser
- [ ] Virtual actuator + actuator bus
- [ ] Sync engine (audio/motion drift measurement)
- [ ] WebSocket bridge + React/D3 dashboard
- [ ] Benchmark harness
- [ ] Serial actuator (real hardware)

## Stack
C++20, CMake, GoogleTest · React, D3.js, Vite · nlohmann/json
