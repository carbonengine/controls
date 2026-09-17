#pragma once

#include "../../StdAfx.h"
#include "../InputEvent.h"

/**
 * @brief Input event that matches a controller hat/d-pad switch reaching a specific position.
 *
 * Supports matching a specific SwitchPosition or Any (any non-center position).
 */
BLUE_CLASS( ControllerSwitchInputEvent ) :
	public InputEvent
{
public:

	EXPOSE_TO_BLUE();

	ControllerSwitchInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc InputEvent::Own() */
	void Own( Events::State& state ) override;

	/** @copydoc InputEvent::JustMatched() */
	bool JustMatched() override;

private:
	/** @copydoc InputEvent::BeforeEvaluate() */
	void BeforeEvaluate() override;

	/** @copydoc InputEvent::Evaluate() */
	bool Evaluate( const Events::State& state ) override;

	/** @copydoc InputEvent::AcceptsElement() */
	bool AcceptsElement( DeviceEnums::InputElementDescriptor element ) const override;

	Events::SwitchPosition m_event{ Events::SwitchPosition::Any };    ///< Target switch position to match.
	Events::SwitchPosition m_state{ Events::SwitchPosition::Center }; ///< Currently matched switch position.
	Events::SwitchPosition m_previousState{ Events::SwitchPosition::Center }; ///< Last matched switch position.
};

TYPEDEF_BLUECLASS( ControllerSwitchInputEvent );
