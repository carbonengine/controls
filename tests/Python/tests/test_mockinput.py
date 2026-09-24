import unittest

from _util import MockDeviceTestCase, carbon_controls


class DeviceConnectionTest(MockDeviceTestCase):

    def test_deviceIsListedAndActive(self):
        deviceIds = [device.deviceID for device in self.controlManager.devices]
        self.assertIn(self.deviceId, deviceIds)
        self.assertTrue(self.controlManager.MockIsDeviceActive(self.deviceId))

    def test_activeDeviceLostCallback(self):
        lost = []
        self.controlManager.activeDeviceLostCallback = lambda deviceId: lost.append(deviceId)
        self.controlManager.MockRemoveDevice(self.deviceId)
        self.controlManager.Update()
        self.assertEqual(lost, [self.deviceId])


class ButtonInputTest(MockDeviceTestCase):

    def test_pressed(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceSouth, carbon_controls.ButtonState.Pressed))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceSouth)

        self.controlManager.MockSetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)

        self.controlManager.MockSetButton(self.deviceId, button, False)
        self.controlManager.Update()
        self.assertEqual(len(calls), 1)

    def test_held(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceEast, carbon_controls.ButtonState.Held))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceEast)

        self.controlManager.MockSetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)

        self.controlManager.MockAdvanceTimeMs(self.controlManager.holdTimeMs + 1)
        self.controlManager.Update()
        self.assertEqual(len(calls), 1)

    def test_pressedNotTriggeredAfterHold(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceWest, carbon_controls.ButtonState.Pressed))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceWest)

        self.controlManager.MockSetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.controlManager.MockAdvanceTimeMs(self.controlManager.holdTimeMs + 1)
        self.controlManager.MockSetButton(self.deviceId, button, False)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)


if __name__ == "__main__":
    unittest.main()
