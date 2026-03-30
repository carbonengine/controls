#include "GamePadDirectionPadInputEvent.h"

GamePadDirectionPadInputEvent::GamePadDirectionPadInputEvent( IRoot* lockobj )
{
}

bool GamePadDirectionPadInputEvent::Match( Events::State state )
{
	uint32_t button = static_cast<uint32_t>( m_button );

	if( ( button & static_cast<uint32_t>( DirectionPadButtonType::Up ) ) != 0 && state.gamePadState.dpadUp.state == m_event )
	{
		return true;
	}
	else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::Down ) ) != 0 && state.gamePadState.dpadDown.state == m_event )
	{
		return true;
	}
	else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::Left ) ) != 0 && state.gamePadState.dpadLeft.state == m_event )
	{
		return true;
	}
	else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::Right ) ) != 0 && state.gamePadState.dpadRight.state == m_event )
	{
		return true;
	}
		else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::UpLeft ) ) != 0 && state.gamePadState.dpadUp.state == m_event && state.gamePadState.dpadLeft.state == m_event )
		{
			return true;
		}
		else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::UpRight ) ) != 0 && state.gamePadState.dpadUp.state == m_event && state.gamePadState.dpadRight.state == m_event )
		{
			return true;
		}
		else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::DownLeft ) ) != 0 && state.gamePadState.dpadDown.state == m_event && state.gamePadState.dpadLeft.state == m_event )
		{
			return true;
		}
		else if( ( button & static_cast<uint32_t>( DirectionPadButtonType::DownRight ) ) != 0 && state.gamePadState.dpadDown.state == m_event && state.gamePadState.dpadRight.state == m_event )
		{
			return true;
		} 
	return false;
}