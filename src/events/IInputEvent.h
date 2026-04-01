#pragma once
#include "../StdAfx.h"

namespace Events
{
  
enum class ButtonState
{
	Up, 
	Down,
	Released,
	Held,
	Pressed
};

enum class Side
{
	Left,
	Right
};

struct Button
{
	uint32_t buttonId; 
	bool _pressed; // device internal button state
	ButtonState state; // the interpreted button state
	std::chrono::steady_clock::time_point m_stateChangeTime; // The time at which the button state last changed
};

struct Trigger
{
	// 0.0 to 1.0, where 1.0 is fully pressed
	float amountPressed;
};

struct ThumbStick
{
	float x;
	float y;
	Button button;
};

struct DPad
{
	Button up;
	Button down;
	Button left;
	Button right;
};

struct FlightStickState
{
	float yaw;
	float pitch;
	float roll;
	Button firePrimary;
	Button fireSecondary;
};

struct GamePadState
{
	Button view;
	Button menu;
	Button a;
	Button b;
	Button x;
	Button y;
	Button leftShoulder;
	Button rightShoulder;
	Button dpadUp;
	Button dpadDown;
	Button dpadLeft;
	Button dpadRight;

	Trigger leftTrigger;
	Trigger rightTrigger;
	ThumbStick leftThumbstick;
	ThumbStick rightThumbstick;
};

struct BatteryState
{
	float remainingCapacity;
	float fullChargeCapacity;
	bool charging;
};

enum class SwitchPosition :uint32_t
{
	Center,
	Up,
	UpRight,
	Right,
	DownRight,
	Down,
	DownLeft,
	Left,
	UpLeft
};

struct ControllerState
{
	std::vector<Button> buttons;
	std::vector<float> axis;
	std::vector<SwitchPosition> switches;
};

struct State
{
	GamePadState gamePadState;
	FlightStickState flightStickState;
	BatteryState batteryState;
	ControllerState controllerState;
};

extern const Be::VarChooser ButtonStateChooser[];
extern const Be::VarChooser SideChooser[];
}

// Interface to match the state from a device
BLUE_INTERFACE( IInputEvent ) :
	public IRoot
{
public:
	virtual bool Match( Events::State state ) = 0;
};
BLUE_DECLARE_INTERFACE( IInputEvent );
BLUE_DECLARE_IVECTOR( IInputEvent );