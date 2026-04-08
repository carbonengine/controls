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


print(f"Registered {len(controller.activeDevice.triggers)} triggers. Polling...")

while True:
    controller.Update()
    blue.os.Pump() 
