# ESPHome `sequence` component

Build multi-stage "sequential switches" from a single button with almost no
boilerplate. One press turns on one light, the next press adds another, the next
turns everything off — and it loops. Each stage is just a list of switches or
lights to keep active; an optional idle timeout drops the sequence back to off.

For anything fancier, a stage can run your own actions on enter, and you can
define what "off" does.

## Install

Point ESPHome at this repository:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/4ib3r/esphome-sequence
    components: [sequence]
```

To hack on it locally, use the checked-out copy instead:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [sequence]
```

## Quick start

```yaml
sequence:
  - id: living_room
    timeout: 15s
    stages:
      - lights: [l1]          # 1st press
      - lights: [l1, l2]      # 2nd press
      # 3rd press wraps back to off

binary_sensor:
  - platform: gpio
    pin: GPIO0
    name: "Cycle button"
    on_press:
      - sequence.advance: living_room
```

Each stage is **exclusive**: entering it turns off every actuator the sequence
knows about, then turns on that stage's set. You never get leftovers from a
previous stage.

## Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `id` | ID | — | **Required.** Id used by the actions. |
| `stages` | list | — | **Required.** Ordered list of stages (see below). |
| `timeout` | time | — | Idle timeout. Re-armed on every advance. See *Timeout*. |
| `loop` | bool | `true` | After the last stage, wrap back to off. If `false`, stay on the last stage. |
| `on_off` | actions | — | Actions to run whenever the sequence drops to off. |

### Stage options

A stage needs **at least one** of these:

| Option | Type | Description |
|--------|------|-------------|
| `switches` | list of IDs | `switch` entities to turn on while this stage is active. |
| `lights` | list of IDs | `light` entities to turn on while this stage is active. |
| `on_enter` | actions | Custom actions run when this stage becomes active (after its switches/lights are set). |

Simple and custom modes combine freely — list some lights *and* run an
`on_enter` block in the same stage.

## Actions

| Action | Description |
|--------|-------------|
| `sequence.advance: <id>` | Move to the next stage (or off after the last / after a timeout). |
| `sequence.off: <id>` | Turn everything off and return to stage 0 (e.g. from a separate button). |
| `sequence.reset: <id>` | Alias of `sequence.off`. |
| `sequence.set_stage:` | Jump straight to a stage. `0` = off, `1..N` = stage. Templatable. |

```yaml
- sequence.set_stage:
    id: living_room
    stage: 2
```

## Timeout

`timeout` is re-armed on every `advance`. When it expires the sequence is
**armed to turn off**: the *next* `advance` drops everything to off instead of
moving forward. Press within the window and the sequence continues as normal.

```text
advance → stage 1
(15 s pass, no press)
advance → off
```

This matches a common wall-switch feel: tap through the stages quickly, or come
back later and a single tap clears the room. Want hands-off auto-off instead?
Wrap a `sequence.reset` in an `on_press` + delay, or call it from an
`interval`/automation.

## Advanced example

```yaml
sequence:
  - id: living_room
    timeout: 15s
    stages:
      - lights: [l1]
      - lights: [l1, l2]
      - lights: [l1]
        on_enter:
          - switch.turn_on: relay
    on_off:
      - switch.turn_off: relay
      - logger.log: "Room sequence cleared"
```

See [`example.yaml`](example.yaml) for a full, compilable configuration.
