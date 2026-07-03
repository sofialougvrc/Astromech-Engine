# ACE Sequence DSL

Sequence files end in `.seq.json` and compile into one timed event queue.

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
    }
  ],
  "sync_tolerance_ms": 10
}
```

Required fields:

- `name`: human-readable sequence id.
- `tracks`: list of actuator tracks.
- `track.actuator`: actuator id routed by `ActuatorBus`.
- `track.type`: actuator family, such as `servo`, `light`, or `audio`.
- `event.t_ms`: monotonic offset from sequence start.
- `event.action`: backend-specific command verb.

Any extra scalar event fields are copied into `ActuatorCommand::params`.
