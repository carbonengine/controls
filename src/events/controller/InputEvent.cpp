#include "InputEvent.h"

InputEvent::InputEvent( IRoot* lockobj )
{
}

bool InputEvent::Match( const Events::State& state )
{
	m_matched = Evaluate( state );
	return m_matched;
}

bool InputEvent::JustMatched()
{
	// an axis only matches when its value moved past the threshold, so every match is a new event
	return m_matched;
}

bool InputEvent::Evaluate( const Events::State& state )
{
	if( m_attached && state.axis.size() > 0 )
	{
		auto it = state.axis.find( static_cast<uint32_t>( m_element ) + m_index );

		if( it == state.axis.end() )
		{
			CCP_LOGERR( "InputEvent::Match: Could not find axis state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
			return false;
		}

		const auto& axis = it->second;

		if( axis.matched )
		{
			return false; // already matched by another event
		}
		if( !m_initialized )
		{
			m_delta = axis.value - m_value;
			m_value = axis.value;
			m_initialized = true;
		}
		if( std::abs( axis.value - m_value ) < Events::AXIS_THRESHOLD )
		{
			return false; // ignore small changes in axis value
		}

		return true;
	}
	return false;
}

void InputEvent::Own( Events::State& state )
{
	if( !m_attached )
	{
			CCP_LOGERR( "InputEvent::Own: Cannot own axis state because no input element is attached." );
		return;
	}
	auto it = state.axis.find( static_cast<uint32_t>( m_element ) + m_index );

	if( it == state.axis.end() )
	{
		CCP_LOGERR( "InputEvent::Own: Could not find axis state for element %s index %d", DeviceEnums::ToKeyString( m_element ), m_index );
		return;
	}

	auto& axis = it->second;

	m_delta = axis.value - m_value;
	m_value = axis.value;
	axis.matched = true;
}

void InputEvent::AttachTo( const InputElement* input )
{
	m_attached = false;
	m_element = DeviceEnums::InputElementDescriptor::Unknown;
	m_index = 0;
	// check if the input is valid and is an axis
	if( input )
	{
		auto element = input->GetElement();

		switch( element )
		{
		case DeviceEnums::InputElementDescriptor::LeftStickX:
		case DeviceEnums::InputElementDescriptor::LeftStickY:
		case DeviceEnums::InputElementDescriptor::RightStickX:
		case DeviceEnums::InputElementDescriptor::RightStickY:
		case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
		case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
		case DeviceEnums::InputElementDescriptor::Unknown:
			// valid input element for axis
			break;
		default:
			// invalid input element for axis
			CCP_LOGERR( "ControllerAxisInputEvent::SetInput: Invalid input element for axis: %s. Ignoring the assignment", DeviceEnums::ToKeyString( element ) );
			return;
		}
		m_attached = true;
		m_element = element;
		m_index = input->GetIndex();
	}
}