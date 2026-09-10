#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"
#include "../../InputElement.h"

/**
 * @brief Input event that matches a controller hat/d-pad switch reaching a specific position.
 *
 * Supports matching a specific SwitchPosition or Any (any non-center position).
 */
BLUE_CLASS( ControllerSwitchInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();

	ControllerSwitchInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc IInputEvent::Match() */
	bool Match( const Events::State& state ) override;

	/** @copydoc IInputEvent::Own() */
	void Own( Events::State& state ) override;

	void AttachTo( const InputElement* input );

private:
	DeviceEnums::InputElementDescriptor m_element{ DeviceEnums::InputElementDescriptor::Unknown }; ///< Input element to monitor for switch position changes.
	uint32_t m_index{ 0 }; ///< Index of the input element in the device's element array.
	bool m_attached{ false }; ///< Whether the input element has been attached to a physical input.
	Events::SwitchPosition m_event{ Events::SwitchPosition::Any };    ///< Target switch position to match.
	Events::SwitchPosition m_state{ Events::SwitchPosition::Center }; ///< Last matched switch position.
};

TYPEDEF_BLUECLASS( ControllerSwitchInputEvent );