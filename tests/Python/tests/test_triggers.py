# Copyright © 2026 CCP ehf.

import unittest

from _util import MockDeviceTestCase, carbon_controls

E = carbon_controls.InputElementDescriptor
B = carbon_controls.ButtonState
S = carbon_controls.SwitchPosition


class HeldButtonTest(MockDeviceTestCase):

    def test_heldNonRepeatFiresOnce(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(calls), 1)

        self.Update(5)
        self.assertEqual(len(calls), 1)

    def test_heldRepeatFiresEveryUpdate(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), repeat=True)
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(calls), 1)

        self.Update(5)
        self.assertEqual(len(calls), 6)

    def test_heldRepeatStopsOnRelease(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), repeat=True)
        self.HoldButton(E.FaceSouth)
        self.Update(2)
        self.SetButton(E.FaceSouth, False)
        self.Update(3)
        self.assertEqual(len(calls), 3)

    def test_heldNonRepeatRefiresAfterReleaseAndHoldAgain(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        self.HoldButton(E.FaceSouth)
        self.SetButton(E.FaceSouth, False)
        self.Update()
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(calls), 2)

    def test_heldNotBeforeHoldTime(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), repeat=True)
        self.SetButton(E.FaceSouth, True)
        self.Update()
        self.AdvanceBeforeHoldTime()
        self.Update()
        self.assertEqual(len(calls), 0)

    def test_downRepeatFiresEveryUpdateWhilePressed(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Down), repeat=True)
        self.SetButton(E.FaceSouth, True)
        self.Update()
        # Down requires the button to have been pressed on the previous evaluation as well
        self.assertEqual(len(calls), 0)
        self.Update(3)
        self.assertEqual(len(calls), 3)

    def test_downNonRepeatFiresOnce(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Down))
        self.SetButton(E.FaceSouth, True)
        self.Update(4)
        self.assertEqual(len(calls), 1)

    def test_releasedFiresOnceAfterHold(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Released), repeat=True)
        self.HoldButton(E.FaceSouth)
        self.SetButton(E.FaceSouth, False)
        self.Update(3)
        self.assertEqual(len(calls), 1)

    def test_pressedRepeatFiresOnceOnRelease(self):
        calls = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Pressed), repeat=True)
        self.TapButton(E.FaceSouth)
        self.Update(3)
        self.assertEqual(len(calls), 1)


class ButtonCombinationTest(MockDeviceTestCase):
    """
    A single "FaceSouth held" trigger alongside a "FaceSouth held + DPadDown pressed" combo.
    The desired behavior is that the combo swallows the single trigger when it fires.
    """

    def _AddSingleAndCombo(self, singleRepeat=False):
        def AddCombo():
            return self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))

        def AddSingle():
            return self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), repeat=singleRepeat)

        single = AddSingle()
        combo = AddCombo()
        return single, combo

    def test_comboFires(self):
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(combo), 0)
        self.TapButton(E.DPadDown)
        self.assertEqual(len(combo), 1)

    def test_comboDoesNotFireWithoutHold(self):
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))
        self.SetButton(E.FaceSouth, True)
        self.Update()
        self.TapButton(E.DPadDown)
        self.assertEqual(len(combo), 0)

    def test_comboDoesNotFireWhenDPadPressedBeforeHold(self):
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))
        self.SetButton(E.FaceSouth, True)
        self.Update()
        self.TapButton(E.DPadDown)
        self.AdvancePastHoldTime()
        self.Update(3)
        self.assertEqual(len(combo), 0)

    def test_singleFiresWhenHoldStartsBeforeCombo(self):
        # The single trigger cannot know that a combo is about to follow, so it fires as soon as the hold starts
        single, combo = self._AddSingleAndCombo()
        self.HoldButton(E.FaceSouth)
        self.assertEqual((len(single), len(combo)), (1, 0))

    def test_singleSuppressedOnComboFrame(self):
        single, combo = self._AddSingleAndCombo()
        self.HoldButton(E.FaceSouth)
        self.SetButton(E.DPadDown, True)
        self.Update()
        self.SetButton(E.DPadDown, False)
        self.Update()
        self.assertEqual((len(single), len(combo)), (1, 1))

    def test_singleDoesNotRefireAfterCombo(self):
        single, combo = self._AddSingleAndCombo()
        self.HoldButton(E.FaceSouth)
        self.TapButton(E.DPadDown)
        self.Update(3)
        self.assertEqual((len(single), len(combo)), (1, 1))

    def test_repeatingSingleSuppressedOnlyOnComboFrame(self):
        single, combo = self._AddSingleAndCombo(singleRepeat=True)
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(single), 1)
        self.SetButton(E.DPadDown, True)
        self.Update()
        self.assertEqual(len(single), 2)
        self.SetButton(E.DPadDown, False)
        self.Update()
        self.assertEqual((len(single), len(combo)), (2, 1))
        self.Update()
        self.assertEqual(len(single), 3)

    def test_comboFiresRepeatedlyWhileHolding(self):
        single, combo = self._AddSingleAndCombo()
        self.HoldButton(E.FaceSouth)
        self.TapButton(E.DPadDown)
        self.TapButton(E.DPadDown)
        self.TapButton(E.DPadDown)
        self.assertEqual((len(single), len(combo)), (1, 3))

    def test_twoCombosSharingHeldButton(self):
        down = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))
        up = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadUp, B.Pressed))
        self.HoldButton(E.FaceSouth)
        self.TapButton(E.DPadDown)
        self.assertEqual((len(down), len(up)), (1, 0))
        self.TapButton(E.DPadUp)
        self.assertEqual((len(down), len(up)), (1, 1))

    def test_threeButtonComboBeatsTwoButtonCombo(self):
        two = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.ButtonEvent(E.DPadDown, B.Pressed))
        three = self.AddTrigger(
            self.ButtonEvent(E.FaceSouth, B.Held),
            self.ButtonEvent(E.LeftShoulder, B.Held),
            self.ButtonEvent(E.DPadDown, B.Pressed),
        )
        self.SetButton(E.FaceSouth, True)
        self.SetButton(E.LeftShoulder, True)
        self.Update()
        self.AdvancePastHoldTime()
        self.Update()
        self.TapButton(E.DPadDown)
        self.assertEqual((len(two), len(three)), (0, 1))

    def test_identicalTriggersOnlyOneFires(self):
        first = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        second = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        self.HoldButton(E.FaceSouth)
        self.assertEqual(len(first) + len(second), 1)

    def test_identicalTriggersDoNotBothFireOverTime(self):
        first = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        second = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        self.HoldButton(E.FaceSouth)
        self.Update(3)
        self.assertEqual(len(first) + len(second), 1)

    # Documented limitation: each mock change is a separate state (as with real hardware), so FaceSouth is
    # released while FaceEast is still down and the single trigger wins before the combo can complete.
    def test_simultaneousPressOfComboAndSingleButton(self):
        single = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Pressed))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Pressed), self.ButtonEvent(E.FaceEast, B.Pressed))
        self.SetButton(E.FaceSouth, True)
        self.SetButton(E.FaceEast, True)
        self.Update()
        self.SetButton(E.FaceSouth, False)
        self.SetButton(E.FaceEast, False)
        self.Update()
        self.assertEqual((len(single), len(combo)), (1, 0))


class MixedElementCombinationTest(MockDeviceTestCase):

    def test_switchTriggerFiresOnceOnChange(self):
        calls = self.AddTrigger(self.SwitchEvent(E.DPad, S.Down))
        self.SetSwitch(E.DPad, S.Down)
        self.Update(3)
        self.assertEqual(len(calls), 1)

    def test_switchAnyFiresOnEachPositionChange(self):
        calls = self.AddTrigger(self.SwitchEvent(E.DPad, S.Any))
        self.Update()
        self.SetSwitch(E.DPad, S.Down)
        self.Update()
        self.SetSwitch(E.DPad, S.DownLeft)
        self.Update()
        self.SetSwitch(E.DPad, S.Left)
        self.Update()
        self.assertEqual(len(calls), 3)

    def test_heldButtonAndSwitchComboSuppressesSwitchTrigger(self):
        switchOnly = self.AddTrigger(self.SwitchEvent(E.DPad, S.Down))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.SwitchEvent(E.DPad, S.Down))
        self.HoldButton(E.FaceSouth)
        self.SetSwitch(E.DPad, S.Down)
        self.Update()
        self.assertEqual((len(switchOnly), len(combo)), (0, 1))

    def test_switchTriggerDoesNotFireAfterComboWhileSwitchStaysDown(self):
        switchOnly = self.AddTrigger(self.SwitchEvent(E.DPad, S.Down))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.SwitchEvent(E.DPad, S.Down))
        self.HoldButton(E.FaceSouth)
        self.SetSwitch(E.DPad, S.Down)
        self.Update()
        self.SetButton(E.FaceSouth, False)
        self.Update(3)
        self.assertEqual((len(switchOnly), len(combo)), (0, 1))

    def test_switchTriggerFiresWithoutHeldButton(self):
        switchOnly = self.AddTrigger(self.SwitchEvent(E.DPad, S.Down))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.SwitchEvent(E.DPad, S.Down))
        self.SetSwitch(E.DPad, S.Down)
        self.Update()
        self.assertEqual((len(switchOnly), len(combo)), (1, 0))

    def test_axisTriggerFiresOnlyOnMovement(self):
        calls = self.AddTrigger(self.AxisEvent(E.LeftStickX))
        self.Update()
        self.SetAxis(E.LeftStickX, 0.5)
        self.Update(3)
        self.assertEqual(len(calls), 1)

    def test_axisBelowThresholdIgnored(self):
        calls = self.AddTrigger(self.AxisEvent(E.LeftStickX))
        self.Update()
        self.SetAxis(E.LeftStickX, 0.005)
        self.Update()
        self.assertEqual(len(calls), 0)

    def test_heldButtonAndAxisComboSuppressesAxisTrigger(self):
        axisOnly = self.AddTrigger(self.AxisEvent(E.LeftStickX))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.AxisEvent(E.LeftStickX))
        self.HoldButton(E.FaceSouth)
        self.SetAxis(E.LeftStickX, 0.5)
        self.Update()
        self.assertEqual((len(axisOnly), len(combo)), (0, 1))

    def test_axisTriggerDoesNotReplayComboMovementAfterRelease(self):
        axisOnly = self.AddTrigger(self.AxisEvent(E.LeftStickX))
        combo = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.AxisEvent(E.LeftStickX))
        self.HoldButton(E.FaceSouth)
        self.SetAxis(E.LeftStickX, 0.5)
        self.Update()
        self.SetButton(E.FaceSouth, False)
        self.Update(3)
        self.assertEqual((len(axisOnly), len(combo)), (0, 1))

    def test_buttonSwitchAndAxisComboBeatsSmallerCombos(self):
        buttonAxis = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.AxisEvent(E.LeftStickX))
        buttonSwitch = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.SwitchEvent(E.DPad, S.Down))
        all3 = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held), self.SwitchEvent(E.DPad, S.Down), self.AxisEvent(E.LeftStickX))
        self.HoldButton(E.FaceSouth)
        self.SetSwitch(E.DPad, S.Down)
        self.SetAxis(E.LeftStickX, 0.5)
        self.Update()
        self.assertEqual((len(buttonAxis), len(buttonSwitch), len(all3)), (0, 1, 1))

    def test_axisAndButtonTriggersAreIndependent(self):
        axisOnly = self.AddTrigger(self.AxisEvent(E.LeftStickX))
        buttonOnly = self.AddTrigger(self.ButtonEvent(E.FaceSouth, B.Held))
        self.HoldButton(E.FaceSouth)
        self.SetAxis(E.LeftStickX, 0.5)
        self.Update()
        self.assertEqual((len(axisOnly), len(buttonOnly)), (1, 1))


if __name__ == "__main__":
    unittest.main()
