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
	m_previouslyConditionsMet = m_conditionsMet;
}

bool ControllerButtonInputEvent::JustMatched()
{
	// edges are detected on the raw condition so that losing ownership to another trigger for a
	// while does not look like a fresh match once that trigger lets go
	return m_matched && !m_previouslyConditionsMet;
}

bool ControllerButtonInputEvent::Evaluate( const Events::State& state )
{
	m_conditionsMet = false;
	if( m_attached )
	{
		const auto* button = GetButtonState( state, m_key );
		if( !button )
		{
			return false;
		}

		if( !m_hasTimestamp )
		{
			m_previousStateChangeTimestamp = state.timestamp;
			m_hasTimestamp = true;
		}

		// saturate so that a timestamp going backwards (e.g. a clock source switch) never reads as a long hold
		const uint64_t elapsed = state.timestamp >= m_previousStateChangeTimestamp ? state.timestamp - m_previousStateChangeTimestamp : 0;
		const bool isHeld = elapsed >= Events::g_holdTimeInMicroSeconds;
		const bool pressed = button->pressed;
		const bool wasPressed = m_previouslyPressed;

		bool matched = false;
		switch( m_event )
		{
		case Events::ButtonState::Up:
			matched = !pressed && !wasPressed;
			break;
		case Events::ButtonState::Down:
			matched = pressed && wasPressed;
			break;
		case Events::ButtonState::Released:
			matched = !pressed && wasPressed && isHeld;
			break;
		case Events::ButtonState::Held:
			matched = pressed && wasPressed && isHeld;
			break;
		case Events::ButtonState::Pressed:
			matched = !pressed && wasPressed && !isHeld;
			break;
		default:
			break;
		}

		if( pressed != wasPressed )
		{
			m_previousStateChangeTimestamp = state.timestamp;
			m_previouslyPressed = pressed;
		}

		m_conditionsMet = matched;
		return matched && !button->matched;
	}
	return false;
}

void ControllerButtonInputEvent::Own( Events::State& state, bool combo )
{
	if( !m_attached )
	{
		CCP_LOGERR( "ControllerButtonInputEvent::Own: Cannot own button state because no input element is attached." );
		return;
	}
	if( auto* button = GetButtonState( state, m_key ) )
	{
		button->matched = true;
		button->comboOwned |= combo;
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
