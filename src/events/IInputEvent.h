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

struct Button
{
	uint32_t buttonId; 
	bool _pressed; // device internal button state
	ButtonState state; // the interpreted button state
	std::chrono::steady_clock::time_point m_stateChangeTime; // The time at which the button state last changed
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

struct State
{
	std::vector<Button> buttons;
	std::vector<float> axis;
	std::vector<SwitchPosition> switches;
};

extern const Be::VarChooser ButtonStateChooser[];
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