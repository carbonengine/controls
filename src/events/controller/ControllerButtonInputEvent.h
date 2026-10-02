// Copyright © 2026 CCP ehf.

#pragma once

#include "../../StdAfx.h"
#include "../InputEvent.h"

/**
 * @brief Input event that matches a controller button reaching a specific ButtonState.
 *
 * Tracks press/release transitions and timestamps to distinguish between
 * Pressed (short tap), Held (long press), Released, Up, and Down states.
 */
BLUE_CLASS( ControllerButtonInputEvent ) :
	public InputEvent
{
public:
	EXPOSE_TO_BLUE();

	ControllerButtonInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc InputEvent::Own() */
	void Own( Events::State& state, bool combo ) override;

	/** @copydoc InputEvent::JustMatched() */
	bool JustMatched() override;

private:
	/** @copydoc InputEvent::BeforeEvaluate() */
	void BeforeEvaluate() override;

	/** @copydoc InputEvent::Evaluate() */
	bool Evaluate( const Events::State& state ) override;

	/** @copydoc InputEvent::AcceptsElement() */
	bool AcceptsElement( DeviceEnums::InputElementDescriptor element ) const override;

	bool m_previouslyConditionsMet{ false }; ///< Raw condition result of the evaluation before the most recent one.

	Events::ButtonState m_event{ Events::ButtonState::Pressed }; ///< Target button state to match.
	bool m_conditionsMet = false; ///< Whether the condition was met in the most recent evaluation, regardless of ownership.
	bool m_previouslyPressed = false; ///< Whether the button was pressed on the previous evaluation.
	bool m_hasTimestamp = false; ///< Whether m_previousStateChangeTimestamp has been initialized.
	uint64_t m_previousStateChangeTimestamp = 0; ///< Timestamp of the last press/release transition.
};

TYPEDEF_BLUECLASS( ControllerButtonInputEvent );