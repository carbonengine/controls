#include "InputDevice.h"

namespace
{
Events::Button Merge( const Events::Button currentState, const Events::Button newState )
{
	auto now = std::chrono::steady_clock::now();
	// A button is considered pressed if it is pressed and released within g_holdTimeInMs, otherwise it is considered held. If it was held and then release then it is considered released.
	Events::Button mergedBtn = currentState;
	mergedBtn.matched = false;
	if( !newState._pressed ){
		if( currentState.state == Events::ButtonState::Held )
		{
			// it was held and then released, so we consider it released
			mergedBtn.state = Events::ButtonState::Released;
		}
		else if( currentState._pressed )
		{
			// it was just released, so we consider it pressed
			mergedBtn.state = Events::ButtonState::Pressed;
		}
		else
		{
			mergedBtn.state = Events::ButtonState::Up;
		}
		mergedBtn._pressed = false;
		mergedBtn.m_stateChangeTime = now;
	}
	else
	{
		// either it was just pressed, or it is being held
		mergedBtn._pressed = true;
		if( !currentState._pressed ){
			mergedBtn.state = Events::ButtonState::Down;
			mergedBtn.m_stateChangeTime = now;
		}
		else if( std::chrono::duration_cast<std::chrono::milliseconds>( now - currentState.m_stateChangeTime ).count() >= InputDevice::g_holdTimeInMs  )
		{
			mergedBtn.state = Events::ButtonState::Held;
		}
	}
	return mergedBtn;
}

Events::State Merge( const Events::State& current, const Events::State& update )
{
	Events::State merged{};
	if( current.buttons.size() == update.buttons.size() )
	{
		merged.buttons.reserve( current.buttons.size() );
		for( size_t i = 0; i < current.buttons.size(); ++i )
		{
			merged.buttons.push_back( Merge( current.buttons[i], update.buttons[i] ) );
		}
	}
	else 
	{
		CCP_LOGWARN( "Controller button count changed from %zu to %zu, resetting state", current.buttons.size(), update.buttons.size() );
		merged = update;
		return merged;
	}

	if( current.axis.size() == update.axis.size() )
	{
		merged.axis = update.axis;
	}
	else
	{
		CCP_LOGWARN( "Controller axis count changed from %zu to %zu, resetting state", current.axis.size(), update.axis.size() );
		merged = update;
		return merged;
	}

	if( current.switches.size() == update.switches.size() )
	{
		merged.switches = update.switches;
	}
	else
	{
		CCP_LOGWARN( "Controller switch count changed from %zu to %zu, resetting state", current.switches.size(), update.switches.size() );
		merged = update;
		return merged;
	}

	return merged;
}


BlueStructureDefinition RawDeviceIdDef[] = {
	{ "id", Be::UINT32_1, 0 },
	{ 0 }
};
}

float InputDevice::g_holdTimeInMs = 300.0f; // The time in milliseconds after which a button state changes from Pressed to Held


InputDevice::InputDevice( IRoot* lockobj ) :
	PARENTLOCK( m_rawDeviceId ),
	PARENTLOCK( m_triggers )
{
	m_rawDeviceId.SetStructureDefinition( RawDeviceIdDef );
	m_rawDeviceId.SetDefaultValue( 0 );
	m_triggers.SetNotify( this );
}

void InputDevice::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* theList )
{
	if( theList == &m_triggers )
	{
		m_triggersDirty = true;
	}
}

void InputDevice::SetIdentifier( DeviceEnums::DeviceIdentifier identifier )
{
	m_deviceIdentifier = identifier;
	m_rawDeviceId.clear();

	for( const auto& part : identifier.rawDeviceId.value )
	{
		m_rawDeviceId.Append( &part );
	}
}

uint32_t InputDevice::GetDeviceID() const
{
	return m_deviceIdentifier.deviceID;
}

BlueSharedStringW InputDevice::GetName() const
{
	return m_deviceIdentifier.name;
}

void InputDevice::Update( IInputHandler* inputHandler )
{
	if( m_triggersDirty )
	{
		m_sortedTriggers.clear();
		for( auto& trigger: m_triggers )
		{
			m_sortedTriggers.push_back( trigger );
		}
		std::sort( m_sortedTriggers.begin(), m_sortedTriggers.end(), []( const InputEventTrigger* a, const InputEventTrigger* b ) {
			return a->GetEventCount() > b->GetEventCount();
		} );
		m_triggersDirty = false;
	}
	
	auto state = inputHandler->Update( m_deviceIdentifier.deviceID );
	// merge the state with the current state
	m_currentState = Merge( m_currentState, state );

	for( const auto& trigger : m_sortedTriggers )
	{
		trigger->Process( m_currentState );
	}
}