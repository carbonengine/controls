# carbon-controls

A Carbon engine shared library (`_carbon_controls`) that provides game controller and input device management. It exposes input device discovery, activation, and event-driven input handling (buttons, axes, switches) to Python via the Blue extension system.

## Features

- **Device discovery** — automatically detects connected input devices (gamepads, controllers, etc.)
- **Hot-plug support** — callbacks for device added/removed events
- **Named input elements** — buttons, axes and switches are exposed as canonical elements (`FaceSouth`, `LeftStickX`, `DPad`, ...) that are consistent across platforms and controller families
- **Event triggers** — register callbacks for button presses, axis movement, and switch changes, including multi-input combos
- **Rumble/haptics** — rumble state management per device
- **Background input** — optionally capture input when the application is not focused
- **Cross-platform** — builds on Windows (via the GameInput API) and macOS (via the GameController and CoreHaptics frameworks)
- **Mock input** — a built-in mock input handler used to drive the Python test suite without real hardware

## Dependencies

| Dependency | Version | Notes |
|---|---|---|
| `carbon-core` | >= 3.0.0 | Core Carbon engine library |
| `carbon-blue` | >= 6.2.0 | Blue runtime |
| `carbon-blueexposure` | >= 2.2.0 | Blue Python binding layer |
| `python3` | >= 3.12.9 | Python development libraries |
| `gameinput` | >= 3.1.26100.6879 | Microsoft GameInput SDK (Windows only) |
| `gtest` | | Test adapter (tests only) |
| `carbon-exefile` | >= 4.1.1 | Host tool used to run the Python tests |

On Windows the GameInput runtime (GameInputRedist) must be installed on the machine for gamepad support. If it is missing, a warning is logged and no devices will be reported.

All dependencies are managed via [VCPKG](https://learn.microsoft.com/en-us/vcpkg/) with the registry vendored at `vendor/github.com/microsoft/vcpkg`.

## Building

### Prerequisites

- CMake 3.31+ (required by `CMakePresets.json`)
- A C++ compiler with C++17 support
  - Windows: Visual Studio 2026 (MSVC v145 toolset)
  - macOS: Xcode / Apple Clang with Objective-C++ support
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

Besides the `_carbon_controls` module, the build also produces `controller_diagnostic`, a standalone command-line tool ([tools/controller_diagnostic.cpp](tools/controller_diagnostic.cpp)) that talks directly to the platform input handler. It is useful for checking how a controller is detected and mapped without going through Python.

### Tests

Tests are enabled by default (`BUILD_TESTING=ON`). The Python unit tests in [tests/Python/tests](tests/Python/tests) run inside `exefile` against the mock input handler and are reported to CTest through a GoogleTest adapter:

```sh
ctest --test-dir .cmake-build-<preset>
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

# Create the platform input handler and start device enumeration
controlManager.Initialize()

# Call Update() at least once so the device list is populated
controlManager.Update()

# List discovered devices
for device in controlManager.devices:
    family = carbon_controls.DeviceFamily.GetNameFromValue(device.family)
    print(f"{device.name}  (ID: {device.deviceID}, family: {family})")

# Activate a device to start reading its input
device = controlManager.Activate(controlManager.devices[0].deviceID)
print(f"Activated: {device.name}")
print(f"  Buttons: {len(device.buttons)}  Axes: {len(device.axes)}  Switches: {len(device.switches)}")
```

`Activate()` returns `None` if the device is not connected. Call `controlManager.Deactivate(deviceID)` to stop polling a device; this also resets its rumble.

The `ControlManager` exposes:
- `Initialize()`: Creates the platform input handler and starts device enumeration. Must be called before `Update()`
- `Activate(deviceID)` / `Deactivate(deviceID)`: Starts/stops polling a device
- `Update()`: Processes device changes and polls active devices, firing trigger callbacks
- `SetBackgroundEventsEnabled(enabled)`: Enables or disables input capture while the application is not focused
- `holdTimeMs`: The hold threshold used by button events (default 300 ms)
- `devices`: All connected devices
- `activeDevices`: The devices that have been activated

Each device exposes:
- `name`: The name of the device
- `deviceID`: The native OS device id
- `family`: The controller family (`DeviceFamily.Unknown`, `Generic`, `PlayStation`, `Xbox`, `Nintendo`)
- `vendorID` / `productID`: The hardware vendor and product identifiers
- `buttons`: The button elements on the device
- `axes`: The axis elements on the device
- `switches`: The switch (D-pad) elements on the device
- `triggers`: The input event triggers registered on the device
- `rumbleMotorCount`: Number of rumble motors (carbon_controls supports up to 4)
- `hasHighFrequencyRumble`: Indicates if it has high frequency rumble support
- `hasLowFrequencyRumble`: Indicates if it has low frequency rumble support
- `hasLeftTriggerRumble`: Indicates if it has left trigger rumble support
- `hasRightTriggerRumble`: Indicates if it has right trigger rumble support
- `highFrequencyRumble`: Sets the high frequency rumble (0-1), applied on the next call to Update()
- `lowFrequencyRumble`: Sets the low frequency rumble (0-1), applied on the next call to Update()
- `leftTriggerRumble`: Sets the left trigger rumble (0-1), applied on the next call to Update()
- `rightTriggerRumble`: Sets the right trigger rumble (0-1), applied on the next call to Update()
- `ResetRumble()`: Resets all rumble intensities to 0

### Input elements

Each entry in `device.buttons`, `device.axes` and `device.switches` is an `InputElement` with:
- `element`: A canonical `InputElementDescriptor` value, e.g. `FaceSouth`, `LeftShoulder`, `LeftStickX`, `RightTriggerAxis`, `DPad`
- `index`: Disambiguates elements that share the same descriptor

Face buttons are named by position (`FaceSouth`, `FaceEast`, `FaceWest`, `FaceNorth`) rather than by label, so the same descriptor refers to the same physical button on Xbox, PlayStation and Nintendo controllers. Use `carbon_controls.InputElementDescriptor.GetNameFromValue(element.element)` to get a readable name.

```python
def find_element(elements, descriptor):
    for element in elements:
        if element.element == descriptor:
            return element
    return None
```

### Registering input event triggers

A trigger holds one or more *event conditions* and a *callback*. The callback fires only when **all** conditions match simultaneously. This lets you listen to simple single-input events as well as complex multi-input combos.

Each event is bound to an input element with `AttachTo(element)`. Elements of the wrong kind (e.g. an axis passed to a button event) are rejected and the event stays unattached; check `event.attached` if in doubt.

Triggers expose:
- `events`: The event conditions that must all match
- `callback`: Called with the list of events when the trigger fires
- `repeat`: If `False` (default) the callback fires once when the events start matching and must stop matching before it can fire again. If `True` it fires for every matching state
- `enabled`: Disabled triggers keep tracking their events but never fire

#### Helper

```python
def register_trigger(device, events, callback, repeat=False):
    """Create a trigger from one or more events, attach a callback, and register it."""
    trigger = carbon_controls.InputEventTrigger()
    for evt in events:
        trigger.events.append(evt)
    trigger.repeat = repeat
    trigger.callback = callback
    device.triggers.append(trigger)
    return trigger
```

#### Button events

```python
FaceSouth = carbon_controls.InputElementDescriptor.FaceSouth

def on_button(events):
    print("FaceSouth pressed")

evt = carbon_controls.ControllerButtonInputEvent()
evt.AttachTo(find_element(device.buttons, FaceSouth))
evt.event = carbon_controls.ButtonState.Pressed

register_trigger(device, [evt], on_button)
```

Available `ButtonState` values:

| State | Meaning |
|---|---|
| `Up` | Button is not pressed |
| `Down` | Button is down, from the first update it is pressed (no time checks) |
| `Pressed` | Button was tapped (went down and up within the hold threshold); matches on the release |
| `Held` | Button has been held longer than the hold threshold |
| `Released` | Button was held for at least the hold threshold and then released; matches on the release |

The hold threshold defaults to 300 ms and can be changed via `controlManager.holdTimeMs`.

#### Axis events

```python
RightStickX = carbon_controls.InputElementDescriptor.RightStickX

def on_axis(events):
    print(f"Right stick X value: {events[0].value}, delta: {events[0].delta}")

evt = carbon_controls.ControllerAxisInputEvent()
evt.AttachTo(find_element(device.axes, RightStickX))

register_trigger(device, [evt], on_axis, repeat=True)
```

The first reading of an axis is used as its baseline and does not fire. After that, changes smaller than 0.01 accumulate until the threshold is crossed, at which point the event matches and `value`/`delta` are updated.

#### Switch (D-pad) events

```python
DPad = carbon_controls.InputElementDescriptor.DPad

def on_switch(events):
    print("D-pad moved up")

evt = carbon_controls.ControllerSwitchInputEvent()
evt.AttachTo(find_element(device.switches, DPad))
evt.event = carbon_controls.SwitchPosition.Up

register_trigger(device, [evt], on_switch)
```

The position to listen for is set through `event`; the current position of the switch is available through `state`.

Available `SwitchPosition` values: `Center`, `Up`, `UpRight`, `Right`, `DownRight`, `Down`, `DownLeft`, `Left`, `UpLeft`, `NonCenter` (any position except center) and `Any` (any position including center; a new match is reported on every position change).

#### Combo trigger — hold one button while pressing another

A trigger's callback fires only when *every* event in its list matches at the same time. Use this to create modifier-style combos:

```python
Element = carbon_controls.InputElementDescriptor

def on_combo(events):
    """Called when the right shoulder is held and the south face button is tapped."""
    print("Combo triggered: RightShoulder + FaceSouth")

# Condition 1: right shoulder must be held down
hold_evt = carbon_controls.ControllerButtonInputEvent()
hold_evt.AttachTo(find_element(device.buttons, Element.RightShoulder))
hold_evt.event = carbon_controls.ButtonState.Down

# Condition 2: south face button (A on Xbox, Cross on PlayStation) must be tapped
press_evt = carbon_controls.ControllerButtonInputEvent()
press_evt.AttachTo(find_element(device.buttons, Element.FaceSouth))
press_evt.event = carbon_controls.ButtonState.Pressed

# Both conditions must be true at the same time for the callback to fire
register_trigger(device, [hold_evt, press_evt], on_combo)
```

You can mix different event types in the same trigger — for example, holding a button while moving an axis:

```python
def on_aim_while_holding(events):
    print("Right stick moved while the left trigger is held")

hold_trigger_evt = carbon_controls.ControllerButtonInputEvent()
hold_trigger_evt.AttachTo(find_element(device.buttons, Element.LeftTrigger))
hold_trigger_evt.event = carbon_controls.ButtonState.Down

axis_evt = carbon_controls.ControllerAxisInputEvent()
axis_evt.AttachTo(find_element(device.axes, Element.RightStickX))

register_trigger(device, [hold_trigger_evt, axis_evt], on_aim_while_holding, repeat=True)
```

#### Trigger priority and ownership

- Triggers with more events are processed first, so a two-button combo takes priority over a single-button listener for the same buttons. Triggers with the same number of events are processed in the order they were added.
- When a trigger fires it *owns* the elements it matched for as long as all of its events keep matching, so smaller triggers on the same elements are blocked while it is active.
- A button owned by a combo (a trigger with more than one event) is *spent*: its release will not produce `Pressed`/`Released` matches on any trigger.
- Every input change is evaluated as its own state, in the order it was reported. Inputs that change "together" are seen one after the other, so a smaller trigger can fire before the state that would complete a larger combo arrives.

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

`deviceRemovedCallback` is only called for devices that are not active; when an active device disconnects `activeDeviceLostCallback` is called instead.

### Full example

See [python/controller_diagnostic.py](python/controller_diagnostic.py) for a complete interactive example that discovers a device, registers button/axis/switch triggers for every element on it, reports rumble capabilities, and polls for input.


