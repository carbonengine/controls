#include "GamePadThumbStickPressedInputEvent.h"

GamePadThumbStickPressedInputEvent::GamePadThumbStickPressedInputEvent( IRoot* lockobj )
{
}

bool GamePadThumbStickPressedInputEvent::Match( Events::State state )
{
	Events::ThumbStick thumbStickState;
	switch( m_side )
	{
	case Events::Side::Left:
		thumbStickState = state.gamePadState.leftThumbstick;
		break;
	case Events::Side::Right:
		thumbStickState = state.gamePadState.rightThumbstick;
		break;
	default:
		return false;
	}
	return thumbStickState.button.state == m_event;
}