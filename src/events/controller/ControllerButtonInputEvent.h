#pragma once

#include "../../StdAfx.h"
#include "../IInputEvent.h"
#include "../../InputElement.h"

/**
 * @brief Input event that matches a controller button reaching a specific ButtonState.
 *
 * Tracks press/release transitions and timestamps to distinguish between
 * Pressed (short tap), Held (long press), Released, Up, and Down states.
 */
BLUE_CLASS( ControllerButtonInputEvent ) :
	public IInputEvent
{
public:

	EXPOSE_TO_BLUE();

	ControllerButtonInputEvent( IRoot* lockobj = nullptr );

	/** @copydoc IInputEvent::Match() */
	bool Match( const Events::State& state ) override;

	/** @copydoc IInputEvent::Own() */
	void Own( Events::State& state ) override;

	/** @copydoc IInputEvent::JustMatched() */
	bool JustMatched() override;

	void AttachTo( const InputElement* input );

private:
	/**
	 * @brief Evaluates the match condition and updates the internal tracking state.
	 * @param state The current device state to evaluate.
	 * @return true if the state satisfies the event condition, false otherwise.
	 */
	bool Evaluate( const Events::State& state );

	bool m_matched{ false };           ///< Result of the most recent evaluation.
	bool m_previouslyMatched{ false }; ///< Result of the evaluation before the most recent one.
	DeviceEnums::InputElementDescriptor m_element{ DeviceEnums::InputElementDescriptor::Unknown }; ///< Input element to monitor for switch position changes.
	uint32_t m_index{ 0 }; ///< Index of the input element in the device's element array.
	bool m_attached{ false }; ///< Whether the input element has been attached to a physical input.

	Events::ButtonState m_event{ Events::ButtonState::Pressed }; ///< Target button state to match.
	bool m_previouslyPressed = false;          ///< Whether the button was pressed on the previous update.
	Events::ButtonState m_currentState = Events::ButtonState::Up; ///< Current button state based on press/release transitions.
	Events::ButtonState m_previousState = Events::ButtonState::Up; ///< Previous button state based on press/release transitions.
	uint64_t m_previousStateChangeTimestamp = 0; ///< Timestamp of the last press/release transition.
};

TYPEDEF_BLUECLASS( ControllerButtonInputEvent );