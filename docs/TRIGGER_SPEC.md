# ACE Trigger Map

Trigger mode is the live remote-control path. A trigger maps one button-style id to immediate actuator commands.

```json
{
  "triggers": [
    {
      "trigger_id": "panel_3_open",
      "commands": [
        { "actuator": "dome_panel_3", "type": "servo", "action": "open" }
      ]
    }
  ]
}
```

At runtime, a trigger is compiled into a sequence where every command is scheduled at `t_ms = 0`. This keeps trigger mode and sequence mode on the same scheduler and actuator bus.
