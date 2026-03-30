import _carbon_controls
import blue
import time
import logging

input("Press enter to start polling for input devices")
controller = _carbon_controls.GetControlManager()
print( dir(controller) )
for device in controller.devices:
    print(f"name: {device.name}")
    print(f"deviceid: {device.deviceID}")
    print(f"deviceType: {device.deviceType}")
    print(f"manufacturer: {device.manufacturer}")
    print(f"product: {device.product}")


index = 0 #input("Press enter to start polling for input events")
controller.Connect( controller.devices[0].deviceID )
while True:
    controller.Update()
    i = 0
    while i < 100:
        blue.os.Pump()
        i += 1 
        
    print( controller.activeDevice.GetStateAsJson() )
