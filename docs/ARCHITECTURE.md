# Astromech Control Engine

ACE is a small C++20 control brain for an R2-style droid build.

- `Scheduler` owns monotonic timing and jitter telemetry.
- `ActuatorBus` routes commands by actuator id.
- `VirtualActuator` keeps an in-memory state for simulation and tests.
- `SerialActuator` preserves the future hardware boundary for Arduino UART/I2C routing.
- `SyncEngine` measures drift across events scheduled for the same timestamp.

The important design rule is that scheduler, trigger, sequence, and sync code do not know whether an actuator is virtual or physical. Swapping backends happens at bus registration time.
