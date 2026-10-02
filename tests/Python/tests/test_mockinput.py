# Copyright © 2026 CCP ehf.

import unittest

from _util import MockDeviceTestCase, carbon_controls


class DeviceConnectionTest(MockDeviceTestCase):

    def test_deviceIsListedAndActive(self):
        deviceIds = [device.deviceID for device in self.controlManager.devices]
        self.assertIn(self.deviceId, deviceIds)
        self.assertTrue(self.mockInput.IsDeviceActive(self.deviceId))

    def test_activeDeviceLostCallback(self):
        lost = []
        self.controlManager.activeDeviceLostCallback = lambda deviceId: lost.append(deviceId)
        self.mockInput.RemoveDevice(self.deviceId)
        self.controlManager.Update()
        self.assertEqual(lost, [self.deviceId])


class ButtonInputTest(MockDeviceTestCase):

    def test_pressed(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceSouth, carbon_controls.ButtonState.Pressed))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceSouth)

        self.mockInput.SetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)

        self.mockInput.SetButton(self.deviceId, button, False)
        self.controlManager.Update()
        self.assertEqual(len(calls), 1)

    def test_held(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceEast, carbon_controls.ButtonState.Held))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceEast)

        self.mockInput.SetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)

        self.mockInput.AdvanceTimeMs(self.controlManager.holdTimeMs + 1)
        self.controlManager.Update()
        self.assertEqual(len(calls), 1)

    def test_pressedNotTriggeredAfterHold(self):
        calls = self.AddTrigger(self.ButtonEvent(carbon_controls.InputElementDescriptor.FaceWest, carbon_controls.ButtonState.Pressed))
        button = self.GetButton(carbon_controls.InputElementDescriptor.FaceWest)

        self.mockInput.SetButton(self.deviceId, button, True)
        self.controlManager.Update()
        self.mockInput.AdvanceTimeMs(self.controlManager.holdTimeMs + 1)
        self.mockInput.SetButton(self.deviceId, button, False)
        self.controlManager.Update()
        self.assertEqual(len(calls), 0)


if __name__ == "__main__":
    unittest.main()
