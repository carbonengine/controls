#include "GamePadTriggerInputEvent.h"


GamePadTriggerInputEvent::GamePadTriggerInputEvent( IRoot* lockobj )
{

}

bool GamePadTriggerInputEvent::Match( Events::State state )
{
	Events::Trigger trigger;

	switch( m_side )
	{
	case Events::Side::Left:
		trigger = state.gamePadState.leftTrigger;
		break;
	case Events::Side::Right:
		trigger = state.gamePadState.rightTrigger;
		break;
	default:
		return false; // Invalid side, so no match
	}

	return trigger.amountPressed >= m_minThreshold && trigger.amountPressed <= m_maxThreshold;
}