#pragma once
#include "../StdAfx.h"

namespace Events
{
const float AXIS_THRESHOLD = 0.005f;
static uint64_t g_holdTimeInMicroSeconds = 300 * 1000; // The time in microseconds after which a button state changes from Pressed to Held

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
	bool pressed = false; 
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
	uint64_t timestamp = 0;
	std::vector<Button> buttons;
	std::vector<Axis> axis;
	std::vector<Switch> switches;
};

struct Rumble
{
	float lowFrequency = 0.0f;
	float highFrequency = 0.0f;
	float leftTrigger = 0.0f;
	float rightTrigger = 0.0f;

	bool empty() const;
};

// returns the current timestamp in microseconds
uint64_t GetTimestamp();

extern const Be::VarChooser ButtonStateChooser[];
}