#include "GamePadButtonInputEvent.h"

GamePadButtonInputEvent::GamePadButtonInputEvent( IRoot* lockobj )
{
}

bool GamePadButtonInputEvent::Match( Events::State state )
{
	switch( m_button )
	{
	case GamePadButtonType::Menu:
		return state.gamePadState.menu.state == m_position;
	case GamePadButtonType::View:
		return state.gamePadState.view.state == m_position;
	case GamePadButtonType::A:
		return state.gamePadState.a.state == m_position;
	case GamePadButtonType::B:
		return state.gamePadState.b.state == m_position;
	case GamePadButtonType::X:
		return state.gamePadState.x.state == m_position;
	case GamePadButtonType::Y:
		return state.gamePadState.y.state == m_position;
	case GamePadButtonType::LeftShoulder:
		return state.gamePadState.leftShoulder.state == m_position;
	case GamePadButtonType::RightShoulder:
		return state.gamePadState.rightShoulder.state == m_position;
	default:
		return false;
	}
}
