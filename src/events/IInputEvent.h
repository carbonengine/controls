#pragma once
#include "../StdAfx.h"

namespace Events
{
const float AXIS_THRESHOLD = 0.005f;

enum class ButtonState
{
	Up, 
	Down,
	Released,
	Held,
	Pressed
};

enum class SwitchPosition : uint32_t
{
	Center,
	Up,
	UpRight,
	Right,
	DownRight,
	Down,
	DownLeft,
	Left,
	UpLeft,
	Any
};

struct Button
{
	bool matched = false;
	bool _pressed = false; // device internal button state
	ButtonState state = ButtonState::Up; // the interpreted button state
	std::chrono::steady_clock::time_point m_stateChangeTime; // The time at which the button state last changed
};

struct Axis
{
	bool matched = false;
	float value = 0.0f;
};

struct Switch
{
	bool matched = false;
	SwitchPosition position = SwitchPosition::Center;
};

struct State
{
	std::vector<Button> buttons;
	std::vector<Axis> axis;
	std::vector<Switch> switches;
};

extern const Be::VarChooser ButtonStateChooser[];
}

// Interface to match the state from a device
BLUE_INTERFACE( IInputEvent ) :
	public IRoot
{
public:
	virtual bool Match( const Events::State& state ) = 0;
	// Marks the parts of the state that matched this event as "owned" by this trigger, so that they won't be considered for identical events 
	virtual void Own( Events::State & state ) = 0; 
};
BLUE_DECLARE_INTERFACE( IInputEvent );
BLUE_DECLARE_IVECTOR( IInputEvent );