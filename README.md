# carbon-controls

A Carbon engine shared library (`_carbon_controls`) that provides game controller and input device management. It exposes input device discovery, activation, and event-driven input handling (buttons, axes, switches) to Python via the Blue extension system.

## Features

- **Device discovery** — automatically detects connected input devices (gamepads, controllers, etc.)
- **Hot-plug support** — callbacks for device added/removed events
- **Event triggers** — register callbacks for button presses, axis movement, and switch changes
- **Rumble/haptics** — rumble state management per device
- **Background input** — optionally capture input when the application is not focused
- **Cross-platform** — builds on Windows (via GameInput API) and macOS

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| `carbon-core` | >= 2.5.0 | Core Carbon engine library |
| `carbon-blueexposure` | >= 2.0.4 | Blue Python binding layer |
| `python3` | >= 3.12.9 | Python development libraries |
| `gameinput` | >= 3.1.x | Microsoft GameInput SDK (Windows only) |

All dependencies are managed via [VCPKG](https://learn.microsoft.com/en-us/vcpkg/) with the registry vendored at `vendor/github.com/microsoft/vcpkg`.

## Building

### Prerequisites

- CMake 3.16+
- A C++ compiler with C++17 support
- [Ninja](https://ninja-build.org/) (recommended generator)

### Configure

List available presets:

```sh
cmake --list-presets
```

Available presets by platform:

| Platform | Presets |
|---|---|
| Windows x64 | `x64-windows-debug`, `x64-windows-internal`, `x64-windows-release`, `x64-windows-trinitydev` |
| macOS arm64 | `arm64-osx-debug`, `arm64-osx-internal`, `arm64-osx-release`, `arm64-osx-trinitydev` |
| macOS x64 | `x64-osx-debug`, `x64-osx-internal`, `x64-osx-release`, `x64-osx-trinitydev` |

Configure with a preset:

```sh
cmake --preset <preset>
```

You can optionally specify a generator:

```sh
cmake -G "Ninja Multi-Config" --preset <preset>
```

### Build

```sh
cmake --build .cmake-build-<preset>
```

## Python Usage

The library is loaded as a Blue extension and exposes controller management to Python.

### Loading the module

```python
import blue

carbon_controls = blue.LoadExtension("_carbon_controls")
```

### Discovering and activating a device

```python
controlManager = carbon_controls.GetControlManager()

# Call Update() at least once so the device list is populated
controlManager.Update()

# List discovered devices
for device in controlManager.devices:
    print(f"{device.name}  (ID: {device.deviceID})")

# Activate a device to start reading its input
device = controlManager.Activate(controlManager.devices[0].deviceID)
print(f"Activated: {device.name}")
print(f"  Buttons: {device.buttonCount}  Axes: {device.axisCount}  Switches: {device.switchCount}")
```

Each device exposes: 
- `name`: The name of the device
- `deviceID`: The native os device id
- `buttonCount`: The number of buttons on the device
- `axisCount`: The number of axes on the device
- `switchCount`: The number of switches on the device
- `hasHighFrequencyRumble`: Indicated if it has high frequency rumble support
- `hasLowFrequencyRumble`: Indicated if it has low frequency rumble support 
- `hasLeftTriggerRumble`: Indicated if it has left trigger rumble support
- `hasRightTriggerRumble`: Indicated if it has right trigger rumble support
- `highFrequencyRumble`: Sets the high frequency rumble (0-1), applied on the next call to Update()
- `lowFrequencyRumble`: Sets the low frequency rumble (0-1), applied on the next call to Update() 
- `leftTriggerRumble`: Sets the left trigger rumble (0-1), applied on the next call to Update()
- `rightTriggerRumble`: Sets the right trigger rumble (0-1), applied on the next call to Update()

### Registering input event triggers

A trigger holds one or more *event conditions* and a *callback*. The callback fires only when **all** conditions match simultaneously on the same polling frame. This lets you listen to simple single-input events as well as complex multi-input combos.

#### Helper

```python
def register_trigger(device, events, callback):
    """Create a trigger from one or more events, attach a callback, and register it."""
    trigger = carbon_controls.InputEventTrigger()
    for evt in events:
        trigger.events.append(evt)
    trigger.callback = callback
    device.triggers.append(trigger)
```

#### Button events

```python
def on_button(events):
    print(f"Button {events[0].buttonIndex} pressed")

evt = carbon_controls.ControllerButtonInputEvent()
evt.buttonIndex = 0
evt.event = carbon_controls.ButtonState.Pressed

register_trigger(device, [evt], on_button)
```

Available `ButtonState` values:

| State | Meaning |
|---|---|
| `Up` | Button is not pressed and was not pressed previously |
| `Down` | Button is continuously held down |
| `Pressed` | Button was tapped (released before the hold threshold) |
| `Held` | Button has been held longer than the hold threshold |
| `Released` | Button was released after being held |

The hold threshold defaults to 300 ms and can be changed via `controlManager.holdTimeInMs`.

#### Axis events

```python
def on_axis(events):
    print(f"Axis {events[0].axisIndex} value: {events[0].value}, delta: {events[0].delta}")

evt = carbon_controls.ControllerAxisInputEvent()
evt.axisIndex = 0

register_trigger(device, [evt], on_axis)
```

Axis changes below a small threshold (0.005) are filtered automatically.

#### Switch (D-pad) events

```python
def on_switch(events):
    print(f"Switch {events[0].switchIndex} moved up")

evt = carbon_controls.ControllerSwitchInputEvent()
evt.switchIndex = 0
evt.position = carbon_controls.SwitchPosition.Up

register_trigger(device, [evt], on_switch)
```

Available `SwitchPosition` values: `Center`, `Up`, `UpRight`, `Right`, `DownRight`, `Down`, `DownLeft`, `Left`, `UpLeft`, `Any`.

#### Combo trigger — hold one button while pressing another

A trigger's callback fires only when *every* event in its list matches at the same time. Use this to create modifier-style combos:

```python
def on_combo(events):
    """Called when button 4 (LB) is held and button 0 (A) is pressed."""
    print("Combo triggered: LB + A")

# Condition 1: button 5 must be held down
hold_evt = carbon_controls.ControllerButtonInputEvent()
hold_evt.buttonIndex = 5  # e.g. right shoulder on XBox One controller
hold_evt.event = carbon_controls.ButtonState.Down

# Condition 2: button 0 must be tapped
press_evt = carbon_controls.ControllerButtonInputEvent()
press_evt.buttonIndex = 0  # e.g. A button on XBox One controller
press_evt.event = carbon_controls.ButtonState.Pressed

# Both conditions must be true on the same frame for the callback to fire
register_trigger(device, [hold_evt, press_evt], on_combo)
```

You can mix different event types in the same trigger — for example, holding a button while moving an axis:

```python
def on_aim_while_holding(events):
    print("Right stick moved while LT is held")

hold_trigger_evt = carbon_controls.ControllerButtonInputEvent()
hold_trigger_evt.buttonIndex = 1  # e.g. X button on XBox One controller 
hold_trigger_evt.event = carbon_controls.ButtonState.Down

axis_evt = carbon_controls.ControllerAxisInputEvent()
axis_evt.axisIndex = 3  # e.g. right stick moved in the horizontal direction on XBox One controller

register_trigger(device, [hold_trigger_evt, axis_evt], on_aim_while_holding)
```

> **Tip:** Triggers with more events are processed first, so a two-button combo will take priority over a single-button listener for the same buttons.

### Polling for input

After registering triggers, call `Update()` each frame to poll devices and fire callbacks:

```python
while True:
    controlManager.Update()
    blue.os.Pump()
```

### Device callbacks

The `ControlManager` supports script callbacks for device lifecycle events:

```python
controlManager.deviceAddedCallback = lambda deviceID: print(f"Device connected: {deviceID}")
controlManager.deviceRemovedCallback = lambda deviceID: print(f"Device disconnected: {deviceID}")
controlManager.activeDeviceLostCallback = lambda deviceID: print(f"Active device lost: {deviceID}")
```

### Full example

See [python/controller_test.py](python/controller_test.py) for a complete interactive example that discovers a device, registers button/axis/switch triggers, and polls for input.


