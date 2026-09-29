import unittest

import blue

carbon_controls = blue.LoadExtension("_carbon_controls")


class _MockInput(object):
    """Forwards mock input calls to the test-only helpers exposed on the ControlManager."""

    def __init__(self, controlManager):
        self._cm = controlManager
        self.connectedDeviceIds = set()

    def AddDevice(self, deviceId, name):
        added = self._cm._MockAddDevice(deviceId, name)
        if added:
            self.connectedDeviceIds.add(deviceId)
        return added

    def RemoveDevice(self, deviceId):
        self.connectedDeviceIds.discard(deviceId)
        return self._cm._MockRemoveDevice(deviceId)

    def SetButton(self, deviceId, element, pressed):
        return self._cm._MockSetButton(deviceId, element, pressed)

    def SetAxis(self, deviceId, element, value):
        return self._cm._MockSetAxis(deviceId, element, value)

    def SetSwitch(self, deviceId, element, position):
        return self._cm._MockSetSwitch(deviceId, element, position)

    def AdvanceTimeMs(self, milliseconds):
        self._cm._MockAdvanceTimeMs(milliseconds)

    def IsDeviceActive(self, deviceId):
        return self._cm._MockIsDeviceActive(deviceId)


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
        self.controlManager._EnableMockInputHandler()
        self.controlManager.Initialize()
        self.mockInput = _MockInput(self.controlManager)
        self._savedHoldTimeMs = self.controlManager.holdTimeMs
        self.assertTrue(self.mockInput.AddDevice(self.deviceId, "Mock Device"))
        self.controlManager.Update()
        self.device = self.controlManager.Activate(self.deviceId)
        self.assertIsNotNone(self.device)

    def tearDown(self):
        # Some tests disconnect the device themselves.
        if self.deviceId in self.mockInput.connectedDeviceIds:
            self.controlManager.Deactivate(self.deviceId)
            self.mockInput.RemoveDevice(self.deviceId)
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
        self.assertTrue(self.mockInput.SetButton(self.deviceId, self.GetButton(descriptor), pressed))

    def SetAxis(self, descriptor, value):
        self.assertTrue(self.mockInput.SetAxis(self.deviceId, self.GetAxis(descriptor), value))

    def SetSwitch(self, descriptor, position):
        self.assertTrue(self.mockInput.SetSwitch(self.deviceId, self.GetSwitch(descriptor), int(position)))

    def Update(self, count=1):
        for _ in range(count):
            self.controlManager.Update()

    def AdvanceBeforeHoldTime(self):
        self.mockInput.AdvanceTimeMs(self.controlManager.holdTimeMs - 1)

    def AdvancePastHoldTime(self):
        self.mockInput.AdvanceTimeMs(self.controlManager.holdTimeMs + 1)

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
