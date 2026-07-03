# ACE Serial Protocol

ACE sends newline-delimited text frames from the Raspberry Pi to an Arduino-style slave board.

Default baud rate is `115200` on both sides:

- ACE: `SerialEndpoint::baud_rate`
- Arduino: `BAUD_RATE` passed to `Serial.begin()`
- Example hardware config: `config/actuators.example.json`

## Frames

```text
PING
STATUS
SERVO:<channel>:<angle-or-action>
LIGHT:<pin>:<state>
AUDIO:<channel>:PLAY:<file>
```

Examples:

```text
SERVO:1:90
SERVO:2:OPEN
LIGHT:13:ON
LIGHT:13:TOGGLE
PING
```

The first Arduino sketch responds with one line per command:

```text
OK:SERVO:1:90
OK:LIGHT:13:ON
OK:PONG
ERR:BAD_FRAME
```

## ACE Mapping

`SerialActuator` converts high-level `ActuatorCommand` objects into serial frames:

- `type=servo`, `angle_deg=90` -> `SERVO:<channel>:90`
- `type=servo`, `action=open` -> `SERVO:<channel>:OPEN`
- `type=light`, `state=on` -> `LIGHT:<channel>:ON`
- `type=audio`, `file=beep.wav` -> `AUDIO:<channel>:PLAY:beep.wav`

The channel comes from `command.params["channel"]` first, then from `SerialEndpoint::channel`.

See `config/actuators.example.json` for the intended actuator-to-device mapping shape.

## Current Hardware Caveat

This protocol compiles now, but it has not been tested against a physical Arduino yet. First hardware validation should be a single LED or unloaded servo before connecting panels, drive motors, or anything with mechanical risk.
