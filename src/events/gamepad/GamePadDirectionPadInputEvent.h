#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"

BLUE_CLASS( GamePadDirectionPadInputEvent ) :
	public IInputEvent
{
public:
	enum class DirectionPadButtonType : uint32_t
	{
		Up = 1 << 0,
		Down = 1 << 1,
		Left = 1 << 2,
		Right = 1 << 3,
		UpLeft = Up | Left,
		UpRight = Up | Right,
		DownLeft = Down | Left,
		DownRight = Down | Right,
		Any = Up | Down | Left | Right
	};

	EXPOSE_TO_BLUE();
	GamePadDirectionPadInputEvent( IRoot* lockobj = nullptr );
	bool Match( Events::State state ) override;

private:
	DirectionPadButtonType m_button{ DirectionPadButtonType::Any };
	Events::ButtonState m_position{ Events::ButtonState::Pressed };
};

TYPEDEF_BLUECLASS( GamePadDirectionPadInputEvent );