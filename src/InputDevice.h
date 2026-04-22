#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "events/Events.h"
#include "events/InputEventTrigger.h"
#include <string>
#include "handlers/IInputHandler.h"

BLUE_DECLARE_VECTOR( InputEventTrigger );

typedef uint32_t RawDeviceIdPart;

BLUE_DECLARE_STRUCTURE_LIST( RawDeviceIdPart );

BLUE_CLASS( InputDevice ) :
	public INotify,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();
	InputDevice( IRoot* lockobj = nullptr );

	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList ) override;

	bool OnModified( Be::Var * value ) override;

	void SetIdentifier( DeviceEnums::DeviceIdentifier identifier );
	void Update( IInputHandler* inputHandler );
	BlueSharedString GetDeviceID() const;
	BlueSharedString GetName() const;

private:
	void UpdateState( const Events::State& state );
	DeviceEnums::DeviceIdentifier m_deviceIdentifier {};
	PRawDeviceIdPartStructureList m_rawDeviceId;

	PInputEventTriggerVector m_triggers;
	std::vector<InputEventTrigger*> m_sortedTriggers;
	Events::State m_currentState;
	bool m_triggersDirty = false;

	Events::Rumble m_rumble{};
	bool m_updateRumble = false;
};

TYPEDEF_BLUECLASS( InputDevice );
