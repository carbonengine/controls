import blue
import time
import logging

carbon_controls = blue.LoadExtension("_carbon_controls")


input("Press enter to start polling for input devices")
controller = carbon_controls.GetControlManager()
print( dir(controller) )

while not controller.devices:
    print( "No input devices found. Retrying in 1 second..." )
    time.sleep( 1 )

print( f"Found {len(controller.devices)} input device(s)." )

if len(controller.devices) > 1:
    print( "Multiple devices found. Please select a device by index:" )
    for i, device in enumerate(controller.devices):
        print( f"{i}: {device.name}" )
    while True:
        selection = input( "Enter device index: " )
        if selection.isdigit() and 0 <= int(selection) < len(controller.devices):
            selected_device = controller.devices[int(selection)]
            break
        else:
            print( "Invalid selection. Please enter a valid device index." )
else:
    # Pick the first device, preferring a gamepad
    selected_device = controller.devices[0]

print(f"Connecting to:")
print(f"name: {selected_device.name}")
controller.Connect( selected_device.deviceID )

# ─── Helper ───────────────────────────────────────────────────────────

def register_trigger( event, callback ):
    """Create an InputEventTrigger, attach the event and callback, and register it."""
    trigger = carbon_controls.InputEventTrigger()
    trigger.events.append( event )
    trigger.callback = callback
    controller.activeDevice.triggers.append( trigger )

# ─── Battery event triggers (common to all device types) ─────────────

def on_battery_low( event ):
    print( "Battery low!" )
def on_battery_charging_started( event ):
    print( "Battery started charging" )
def on_battery_charging_stopped( event ):
    print( "Battery stopped charging" )

battery_low_evt = carbon_controls.BatteryLowEvent()
battery_low_evt.threshold = 0.2
register_trigger( battery_low_evt, on_battery_low )

charging_started_evt = carbon_controls.BatteryChargingEvent()
charging_started_evt.event = carbon_controls.ChargingState.StartedCharging
register_trigger( charging_started_evt, on_battery_charging_started )

charging_stopped_evt = carbon_controls.BatteryChargingEvent()
charging_stopped_evt.event = carbon_controls.ChargingState.StoppedCharging
register_trigger( charging_stopped_evt, on_battery_charging_stopped )

# ─── Device-specific triggers ─────────────────────────────────────────

if selected_device.deviceType == carbon_controls.DeviceType.Controller:
    # ── Flight stick callbacks ────────────────────────────────────────
    def button_callback( event ):
        print( f"Button event: {event[0].buttonIndex}" )
    def axis_callback( event ):
        print( f"Axis event: {event[0].axisIndex}" )
    def switch_callback( event ):
        print( f"Switch event: {event[0].switchIndex}" )

    print( f"Registering {selected_device.buttonCount} button triggers, ")
    for button_index in range( selected_device.buttonCount ):
        evt = carbon_controls.ControllerButtonInputEvent()
        evt.buttonIndex = button_index
        evt.event = carbon_controls.ButtonState.Pressed
        register_trigger( evt, button_callback )
    print( f"{selected_device.axisCount} axis triggers, ")
    for axis_index in range( selected_device.axisCount ):
        evt = carbon_controls.ControllerAxisInputEvent()
        evt.axisIndex = axis_index
        register_trigger( evt, axis_callback )
    print( f"and {selected_device.switchCount} switch triggers." )
    for switch_index in range( selected_device.switchCount ):
        evt = carbon_controls.ControllerSwitchInputEvent()
        evt.switchIndex = switch_index
        evt.position = carbon_controls.SwitchPosition.Up
        register_trigger( evt, switch_callback )

else:
    # ── Gamepad callbacks ─────────────────────────────────────────────

    # Buttons
    def on_button_a_pressed( event ):
        print( "A pressed" )
    def on_button_a_released( event ):
        print( "A released" )
    def on_button_a_held( event ):
        print( "A held" )

    def on_button_b_pressed( event ):
        print( "B pressed" )
    def on_button_b_released( event ):
        print( "B released" )
    def on_button_b_held( event ):
        print( "B held" )

    def on_button_x_pressed( event ):
        print( "X pressed" )
    def on_button_x_released( event ):
        print( "X released" )
    def on_button_x_held( event ):
        print( "X held" )

    def on_button_y_pressed( event ):
        print( "Y pressed" )
    def on_button_y_released( event ):
        print( "Y released" )
    def on_button_y_held( event ):
        print( "Y held" )

    def on_left_shoulder_pressed( event ):
        print( "Left Shoulder pressed" )
    def on_left_shoulder_released( event ):
        print( "Left Shoulder released" )
    def on_left_shoulder_held( event ):
        print( "Left Shoulder held" )

    def on_right_shoulder_pressed( event ):
        print( "Right Shoulder pressed" )
    def on_right_shoulder_released( event ):
        print( "Right Shoulder released" )
    def on_right_shoulder_held( event ):
        print( "Right Shoulder held" )

    def on_menu_pressed( event ):
        print( "Menu pressed" )
    def on_view_pressed( event ):
        print( "View pressed" )

    # DPad
    def on_dpad_up_pressed( event ):
        print( "DPad Up pressed" )
    def on_dpad_down_pressed( event ):
        print( "DPad Down pressed" )
    def on_dpad_left_pressed( event ):
        print( "DPad Left pressed" )
    def on_dpad_right_pressed( event ):
        print( "DPad Right pressed" )

    # Thumbsticks
    def on_left_thumbstick_moved( event ):
        print( "Left thumbstick moved" )
    def on_right_thumbstick_moved( event ):
        print( "Right thumbstick moved" )
    def on_left_thumbstick_pressed( event ):
        print( "Left thumbstick pressed" )
    def on_left_thumbstick_released( event ):
        print( "Left thumbstick released" )
    def on_right_thumbstick_pressed( event ):
        print( "Right thumbstick pressed" )
    def on_right_thumbstick_released( event ):
        print( "Right thumbstick released" )

    # Analog triggers
    def on_left_trigger( event ):
        print( "Left trigger" )
    def on_right_trigger( event ):
        print( "Right trigger" )

    def special_trigger( event ):
        print( "Special trigger activated!" )
        special_trigger

    # ── Gamepad button registration helper ────────────────────────────
    trigger = carbon_controls.InputEventTrigger()
    evt = carbon_controls.GamePadButtonInputEvent()
    evt.button = carbon_controls.GamePadButtonType.A
    evt.event = carbon_controls.ButtonState.Held
    trigger.events.append( evt )
    evt2= carbon_controls.GamePadButtonInputEvent()
    evt2.button = carbon_controls.GamePadButtonType.B
    evt2.event = carbon_controls.ButtonState.Pressed
    trigger.events.append( evt2 )
    trigger.callback = special_trigger
    controller.activeDevice.triggers.append( trigger )

    def register_button( button_type, pressed_cb, released_cb=None, held_cb=None ):
        for state, cb in [
            (carbon_controls.ButtonState.Pressed, pressed_cb),
            (carbon_controls.ButtonState.Released, released_cb),
            (carbon_controls.ButtonState.Held, held_cb),
        ]:
            if cb is not None:
                evt = carbon_controls.GamePadButtonInputEvent()
                evt.button = button_type
                evt.event = state
                register_trigger( evt, cb )

    # register_button( carbon_controls.GamePadButtonType.A,
    #     on_button_a_pressed, on_button_a_released, on_button_a_held )
    register_button( carbon_controls.GamePadButtonType.B,
        on_button_b_pressed, on_button_b_released, on_button_b_held )
    register_button( carbon_controls.GamePadButtonType.X,
        on_button_x_pressed, on_button_x_released, on_button_x_held )
    register_button( carbon_controls.GamePadButtonType.Y,
        on_button_y_pressed, on_button_y_released, on_button_y_held )
    register_button( carbon_controls.GamePadButtonType.LeftShoulder,
        on_left_shoulder_pressed, on_left_shoulder_released, on_left_shoulder_held )
    register_button( carbon_controls.GamePadButtonType.RightShoulder,
        on_right_shoulder_pressed, on_right_shoulder_released, on_right_shoulder_held )
    register_button( carbon_controls.GamePadButtonType.Menu, on_menu_pressed )
    register_button( carbon_controls.GamePadButtonType.View, on_view_pressed )

    # ── DPad triggers ─────────────────────────────────────────────────

    def register_dpad( direction, callback ):
        evt = carbon_controls.GamePadDirectionPadInputEvent()
        evt.button = direction
        evt.event = carbon_controls.ButtonState.Pressed
        register_trigger( evt, callback )

    register_dpad( carbon_controls.DirectionPadButtonType.Up, on_dpad_up_pressed )
    register_dpad( carbon_controls.DirectionPadButtonType.Down, on_dpad_down_pressed )
    register_dpad( carbon_controls.DirectionPadButtonType.Left, on_dpad_left_pressed )
    register_dpad( carbon_controls.DirectionPadButtonType.Right, on_dpad_right_pressed )

    # ── Thumbstick moved triggers ─────────────────────────────────────

    def register_thumbstick_moved( side, callback ):
        evt = carbon_controls.GamePadThumbStickMovedInputEvent()
        evt.side = side
        register_trigger( evt, callback )

    register_thumbstick_moved( carbon_controls.Side.Left, on_left_thumbstick_moved )
    register_thumbstick_moved( carbon_controls.Side.Right, on_right_thumbstick_moved )

    # ── Thumbstick pressed triggers ───────────────────────────────────

    def register_thumbstick_pressed( side, pressed_cb, released_cb=None ):
        for state, cb in [
            (carbon_controls.ButtonState.Pressed, pressed_cb),
            (carbon_controls.ButtonState.Released, released_cb),
        ]:
            if cb is not None:
                evt = carbon_controls.GamePadThumbStickPressedInputEvent()
                evt.side = side
                evt.event = state
                register_trigger( evt, cb )

    register_thumbstick_pressed( carbon_controls.Side.Left,
        on_left_thumbstick_pressed, on_left_thumbstick_released )
    register_thumbstick_pressed( carbon_controls.Side.Right,
        on_right_thumbstick_pressed, on_right_thumbstick_released )

    # ── Analog trigger (LT / RT) triggers ─────────────────────────────

    def register_analog_trigger( side, callback, min_threshold=0.5 ):
        evt = carbon_controls.GamePadTriggerInputEvent()
        evt.side = side
        evt.minThreshold = min_threshold
        register_trigger( evt, callback )

    register_analog_trigger( carbon_controls.Side.Left, on_left_trigger )
    register_analog_trigger( carbon_controls.Side.Right, on_right_trigger )

print(f"Registered {len(controller.activeDevice.triggers)} triggers. Polling...")

while True:
    controller.Update()
    blue.os.Pump() 
