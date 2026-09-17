"""Controller diagnostic driven through the real carbon_controls pipeline.

ControlManager -> InputDevice -> InputEventTrigger -> IInputEvent

Nothing here touches IInputHandler directly. Every button and switch report
below is produced by an actual InputEventTrigger callback firing from inside
ControlManager.Update(), so running this exercises the same code path that
shipping script code uses.
"""

import time
import blue
cc = blue.LoadExtension("_carbon_controls")

POLL_INTERVAL = 50 #ms

# Registered by BLUE_REGISTER_ENUM_EX in InputEventTrigger_Blue.cpp.
# "Up" is deliberately omitted: it means "not pressed", so it fires for every
# other button on nearly every update, making it look like all buttons print.
BUTTON_STATES = ("Up", "Pressed", "Held", "Released")

# Registered by BLUE_REGISTER_ENUM_EX in ControllerSwitchInputEvent_Blue.cpp.
# "Any" is deliberately omitted so it doesn't shadow every concrete position.
SWITCH_POSITIONS = (
    "Center", "Up", "UpRight", "Right", "DownRight",
    "Down", "DownLeft", "Left", "UpLeft",
)

AXIS_REPORT_INTERVAL = 0.1


def describe(device):
    print(device.family)
    return "{0} (id={1}, vendor={2}, product={3}, family={4})".format(
        device.name, device.deviceID, device.vendorID,
        device.productID, cc.DeviceFamily.GetNameFromValue(device.family)
    )


def wait_for_device(manager, timeout=30.0):
    """Pump Update() until ProcessChangedDevices() populates ControlManager.devices."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        manager.Update()
        if len(manager.devices) > 0:
            return list(manager.devices)
        time.sleep(POLL_INTERVAL / 1000.0)
    return []


def choose_device(devices):
    if len(devices) == 1:
        return devices[0]

    print("\nConnected devices:")
    for i, device in enumerate(devices):
        print("  [{0}] {1}".format(i + 1, describe(device)))

    while True:
        answer = input("\nSelect device [1-{0}]: ".format(len(devices))).strip()
        if answer.isdigit() and 1 <= int(answer) <= len(devices):
            return devices[int(answer) - 1]


def add_trigger(device, event, repeat, on_fire):
    """Wire a single IInputEvent into a new InputEventTrigger on the device."""
    trigger = cc.InputEventTrigger()
    trigger.events.append(event)
    trigger.repeat = repeat
    trigger.callback = on_fire
    device.triggers.append(trigger)
    return trigger


def make_button_reporter(label, state_name):
    def on_fire(events):
        print("  button  {0:<22} {1}".format(label, state_name))
    return on_fire


def make_switch_reporter(label, position_name):
    def on_fire(events):
        print("  switch  {0:<22} {1}".format(label, position_name))
    return on_fire


def wire_buttons(device):
    count = 0
    for element in device.buttons:
        label = cc.InputElementDescriptor.GetNameFromValue(element.element)
        for state_name in BUTTON_STATES:
            event = cc.ControllerButtonInputEvent()
            event.AttachTo(element)
            event.event = getattr(cc.ButtonState, state_name)
            add_trigger(device, event, repeat=False, on_fire=make_button_reporter(label, state_name))
            count += 1
    return count


def wire_switches(device):
    count = 0
    for element in device.switches:
        label = cc.InputElementDescriptor.GetNameFromValue(element.element)
        for position_name in SWITCH_POSITIONS:
            event = cc.ControllerSwitchInputEvent()
            event.AttachTo(element)
            event.event = getattr(cc.SwitchPosition, position_name)
            add_trigger(device, event, repeat=False, on_fire=make_switch_reporter(label, position_name))
            count += 1
    return count


def wire_axes(device):
    """Axis events expose .value/.delta, so keep handles and poll them for display."""
    handles = []
    for element in device.axes:
        event = cc.ControllerAxisInputEvent()
        event.AttachTo(element)
        add_trigger(device, event, repeat=True, on_fire=lambda events: None)
        handles.append((str(element.element), event))
    return handles


def axis_bar(value, width=20):
    half = width // 2
    filled = int(max(-1.0, min(1.0, value)) * half)
    cells = []
    for i in range(-half, half):
        if i == 0:
            cells.append("|")
        elif (filled >= 0 and 0 <= i < filled) or (filled < 0 and filled <= i < 0):
            cells.append("=")
        else:
            cells.append(" ")
    return "[{0}]".format("".join(cells))


AXIS_CHANGE_EPSILON = 0.001


def report_axes(handles, last_values):
    for label, event in handles:
        value = event.value
        if abs(value - last_values.get(label, 0.0)) < AXIS_CHANGE_EPSILON:
            continue
        last_values[label] = value
        print("  axis    {0:<22} {1} {2:+.3f} (delta {3:+.3f})".format(
            label, axis_bar(value), value, event.delta))


def report_rumble_capacity(device):
    if device.rumbleMotorCount <= 0:
        print("Rumble: not supported\n")
        return
    print("Rumble: motors={0} low={1} high={2} leftTrigger={3} rightTrigger={4}\n".format(
        device.rumbleMotorCount,
        device.hasLowFrequencyRumble,
        device.hasHighFrequencyRumble,
        device.hasLeftTriggerRumble,
        device.hasRightTriggerRumble,
    ))


def main():
    manager = cc.GetControlManager()
    manager.SetBackgroundEventsEnabled(True)
    manager.holdTimeMs = 500

    manager.deviceAddedCallback = lambda device_id: print("[+] added:   {0}".format(device_id))
    manager.deviceRemovedCallback = lambda device_id: print("[-] removed: {0}".format(device_id))
    manager.activeDeviceLostCallback = lambda device_id: print("[!] active device lost: {0}".format(device_id))

    print("Waiting for a controller...")
    devices = wait_for_device(manager)
    if not devices:
        print("No controller detected.")
        return 1

    device = choose_device(devices)
    print("\nSelected: {0}".format(describe(device)))

    active = manager.Activate(device.deviceID)
    if active is None:
        print("Activate() failed - device is not connected.")
        return 1

    button_count = wire_buttons(active)
    switch_count = wire_switches(active)
    axis_handles = wire_axes(active)

    print("Wired {0} button, {1} switch and {2} axis triggers.".format(
        button_count, switch_count, len(axis_handles)))
    report_rumble_capacity(active)
    print("Press controller inputs. Ctrl+C to quit.\n")

    last_axis_report = 0.0
    last_axis_values = {}
    try:
        while True:
            # One call drives the whole pipeline; triggers fire from inside here.
            manager.Update()

            now = time.time()
            if now - last_axis_report >= AXIS_REPORT_INTERVAL:
                report_axes(axis_handles, last_axis_values)
                last_axis_report = now

            time.sleep(POLL_INTERVAL/1000.0)
    except KeyboardInterrupt:
        print("\nShutting down.")
    finally:
        active.ResetRumble()
        manager.Deactivate(device.deviceID)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
