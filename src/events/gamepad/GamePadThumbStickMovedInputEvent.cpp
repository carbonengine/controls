#include "GamePadThumbStickMovedInputEvent.h"

GamePadThumbStickMovedInputEvent::GamePadThumbStickMovedInputEvent( IRoot* lockobj )
{
}

bool GamePadThumbStickMovedInputEvent::Match( Events::State state )
{
	Events::ThumbStick thumbStickState{};
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
	return thumbStickState.x != 0.5f || thumbStickState.y != 0.5f;
}