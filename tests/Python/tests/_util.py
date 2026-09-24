import unittest

import blue

carbon_controls = blue.LoadExtension("_carbon_controls")


class MockDeviceTestCase(unittest.TestCase):
    """
    Connects and activates a fake gamepad through the mock input handler for each test.

    The ControlManager is a module-level singleton, so every test gets a unique device ID and
    the device is removed again in tearDown to keep tests independent.
    """

    _deviceCounter = 0

    def setUp(self):
        MockDeviceTestCase._deviceCounter += 1
        self.deviceId = "mock-device-%d" % MockDeviceTestCase._deviceCounter
        self.controlManager = carbon_controls.GetControlManager()
        self._savedHoldTimeMs = self.controlManager.holdTimeMs
        self.assertTrue(self.controlManager.MockAddDevice(self.deviceId, "Mock Device"))
        self.controlManager.Update()
        self.device = self.controlManager.Activate(self.deviceId)
        self.assertIsNotNone(self.device)

    def tearDown(self):
        self.controlManager.Deactivate(self.deviceId)
        self.controlManager.MockRemoveDevice(self.deviceId)
        self.controlManager.Update()
        self.controlManager.holdTimeMs = self._savedHoldTimeMs

    def AddTriggerEx(self, *events, repeat=False):
        """Like AddTrigger, but also returns the trigger so tests can toggle ``enabled`` or add events later."""
        calls = []
        trigger = carbon_controls.InputEventTrigger()
        for event in events:
            trigger.events.append(event)
        trigger.repeat = repeat
        trigger.callback = lambda matchedEvents: calls.append(matchedEvents)
        self.device.triggers.append(trigger)
        return trigger, calls

    def RemoveTrigger(self, trigger):
        self.device.triggers.remove(trigger)

    def AppendEvents(self, trigger, *events):
        """Adds events to a trigger that is already attached to the device."""
        for event in events:
            trigger.events.append(event)

    def GetButton(self, descriptor):
        return self._FindElement(self.device.buttons, descriptor)

    def GetAxis(self, descriptor):
        return self._FindElement(self.device.axes, descriptor)

    def GetSwitch(self, descriptor):
        return self._FindElement(self.device.switches, descriptor)

    def AddTrigger(self, *events, repeat=False):
        return self.AddTriggerEx(*events, repeat=repeat)[1]

    def ButtonEvent(self, descriptor, state):
        event = carbon_controls.ControllerButtonInputEvent()
        event.AttachTo(self.GetButton(descriptor))
        event.event = state
        return event

    def AxisEvent(self, descriptor):
        event = carbon_controls.ControllerAxisInputEvent()
        event.AttachTo(self.GetAxis(descriptor))
        return event

    def SwitchEvent(self, descriptor, position):
        event = carbon_controls.ControllerSwitchInputEvent()
        event.AttachTo(self.GetSwitch(descriptor))
        event.event = position
        return event

    def SetButton(self, descriptor, pressed):
        self.assertTrue(self.controlManager.MockSetButton(self.deviceId, self.GetButton(descriptor), pressed))

    def SetAxis(self, descriptor, value):
        self.assertTrue(self.controlManager.MockSetAxis(self.deviceId, self.GetAxis(descriptor), value))

    def SetSwitch(self, descriptor, position):
        self.assertTrue(self.controlManager.MockSetSwitch(self.deviceId, self.GetSwitch(descriptor), int(position)))

    def Update(self, count=1):
        for _ in range(count):
            self.controlManager.Update()

    def AdvanceBeforeHoldTime(self):
        self.controlManager.MockAdvanceTimeMs(self.controlManager.holdTimeMs - 1)

    def AdvancePastHoldTime(self):
        self.controlManager.MockAdvanceTimeMs(self.controlManager.holdTimeMs + 1)

    def TapButton(self, descriptor):
        self.SetButton(descriptor, True)
        self.Update()
        self.SetButton(descriptor, False)
        self.Update()

    def HoldButton(self, descriptor):
        self.SetButton(descriptor, True)
        self.Update()
        self.AdvancePastHoldTime()
        self.Update()

    def _FindElement(self, elements, descriptor):
        for element in elements:
            if element.element == descriptor:
                return element
        self.fail("Element %s not found on mock device" % descriptor)
