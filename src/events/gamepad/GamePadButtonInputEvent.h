#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( GamePadButtonInputEvent ) :
	public IInputEvent
{
public:
	enum class GamePadButtonType
	{
		None,
		Menu,
		View,
		A,
		B,
		X,
		Y,
		LeftShoulder,
		RightShoulder
	};

	EXPOSE_TO_BLUE();
	GamePadButtonInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	GamePadButtonType m_button{ GamePadButtonType::None };
	Events::ButtonState m_event{ Events::ButtonState::Pressed };
};

TYPEDEF_BLUECLASS( GamePadButtonInputEvent );