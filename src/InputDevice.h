#pragma once
#include "StdAfx.h"
#include "DeviceEnums.h"
#include "events/InputEventTrigger.h"
#include <string>
#include "handlers/IInputHandler.h"

BLUE_DECLARE_VECTOR( InputEventTrigger );

typedef uint32_t RawDeviceIdPart;

BLUE_DECLARE_STRUCTURE_LIST( RawDeviceIdPart );

BLUE_CLASS( InputDevice ) : public IListNotify
{
public:
	EXPOSE_TO_BLUE();
	InputDevice( IRoot* lockobj = nullptr );

	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList );
	void SetIdentifier( DeviceEnums::DeviceIdentifier identifier );
	void Update( IInputHandler* inputHandler );
	uint32_t GetDeviceID() const;
	BlueSharedStringW GetName() const;
	
	static float g_holdTimeInMs; // The time in milliseconds after which a button state changes from Pressed to Held

private:

	DeviceEnums::DeviceIdentifier m_deviceIdentifier {};
	PRawDeviceIdPartStructureList m_rawDeviceId;

	PInputEventTriggerVector m_triggers;
	std::vector<InputEventTrigger*> m_sortedTriggers;
	Events::State m_currentState;
	bool m_triggersDirty = false;
};

TYPEDEF_BLUECLASS( InputDevice );
