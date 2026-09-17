#include "ControllerButtonInputEvent.h"

namespace
{

Events::Button* GetButtonState( Events::State& state, DeviceEnums::ElementKey key )
{
	auto it = state.buttons.find( key );
	if( it != state.buttons.end() )
	{
		return &it->second;
	}
	CCP_LOGERR( "Button state not found for key %s", DeviceEnums::ToKeyString( key.descriptor ) );
	return nullptr;
}

const Events::Button* GetButtonState( const Events::State& state, DeviceEnums::ElementKey key )
{
	auto it = state.buttons.find( key );
	if( it != state.buttons.end() )
	{
		return &it->second;
	}
	CCP_LOGERR( "Button state not found for key %s", DeviceEnums::ToKeyString( key.descriptor ) );
	return nullptr;
}
}


ControllerButtonInputEvent::ControllerButtonInputEvent( IRoot* lockobj ) :
	InputEvent( lockobj )
{
}

void ControllerButtonInputEvent::BeforeEvaluate()
{
	m_previouslyMatched = m_matched;
}

bool ControllerButtonInputEvent::JustMatched()
{
	return m_matched && !m_previouslyMatched;
}

bool ControllerButtonInputEvent::Evaluate( const Events::State& state )
{
	if( m_attached )
	{
		const auto* button = GetButtonState( state, m_key );
		if( !button )
		{
			return false;
		}

		if( m_previousStateChangeTimestamp == 0 )
		{
			m_previousStateChangeTimestamp = state.timestamp;
		}

		bool pressed = button->pressed;
		bool matched = false;
		bool isHeld = state.timestamp - m_previousStateChangeTimestamp >= Events::g_holdTimeInMicroSeconds;
		switch( m_event )
		{
		case Events::ButtonState::Up:
			matched = !pressed && !m_previouslyPressed;
			break;
		case Events::ButtonState::Down:
			matched = pressed && m_previouslyPressed;
			break;
		case Events::ButtonState::Released:
			matched = !pressed && m_previouslyPressed && isHeld;
			break;
		case Events::ButtonState::Held:
			matched = pressed && m_previouslyPressed && isHeld;
			break;
		case Events::ButtonState::Pressed:
			matched = !pressed && m_previouslyPressed && !isHeld;
			break;
		default:
			break;
		}

		if( m_previouslyPressed != pressed )
		{
			m_previousStateChangeTimestamp = state.timestamp;
			m_previouslyPressed = pressed;
		}

		// another trigger already consumed this button for this state. The tracking state above is still
		// updated so that this event does not desync from the actual button, but it cannot match.
		if( button->matched )
		{
			return false;
		}

		return matched;
	}
	return false;
}

void ControllerButtonInputEvent::Own( Events::State& state )
{
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerButtonInputEvent::Own: Cannot own button state because no input element is attached." );
		return;
	}
	if( auto* button = GetButtonState( state, m_key ) )
	{
		button->matched = true;
	}
}

bool ControllerButtonInputEvent::AcceptsElement( DeviceEnums::InputElementDescriptor element ) const
{
	switch( element )
	{
	case DeviceEnums::InputElementDescriptor::LeftStickX:
	case DeviceEnums::InputElementDescriptor::LeftStickY:
	case DeviceEnums::InputElementDescriptor::RightStickX:
	case DeviceEnums::InputElementDescriptor::RightStickY:
	case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
	case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
	case DeviceEnums::InputElementDescriptor::DPad:
		CCP_LOGERR( "ControllerButtonInputEvent::AttachTo: Invalid input element for button: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
		return false;
	default:
		return true;
	}
}
