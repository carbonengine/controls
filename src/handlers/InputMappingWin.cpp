// Copyright © 2026 CCP ehf.

#ifdef WIN32
#include "InputMappingWin.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace GameInput::v3;

namespace
{
// Canonical ordering of the gamepad view's buttons.
//
// GameInputGamepadInfo::supportedLayout advertises exactly which of these a device
// actually has - even a generic pad reports a layout - so the published button list is
// built from that mask instead of a hard-coded set.
struct GamepadButtonMapping
{
	GameInputGamepadButtons mask;
	DeviceEnums::InputElementDescriptor element;
};

constexpr GamepadButtonMapping GAMEPAD_BUTTONS[] = {
	{ GameInputGamepadMenu, DeviceEnums::InputElementDescriptor::Start },
	{ GameInputGamepadView, DeviceEnums::InputElementDescriptor::Select },
	{ GameInputGamepadA, DeviceEnums::InputElementDescriptor::FaceSouth },
	{ GameInputGamepadB, DeviceEnums::InputElementDescriptor::FaceEast },
	{ GameInputGamepadX, DeviceEnums::InputElementDescriptor::FaceWest },
	{ GameInputGamepadY, DeviceEnums::InputElementDescriptor::FaceNorth },
	// C and Z occupy no canonical slot, so they have no canonical identity. They are still
	// published so state indices stay aligned with the rest of the layout.
	{ GameInputGamepadC, DeviceEnums::InputElementDescriptor::Unknown },
	{ GameInputGamepadZ, DeviceEnums::InputElementDescriptor::Unknown },
	{ GameInputGamepadLeftShoulder, DeviceEnums::InputElementDescriptor::LeftShoulder },
	{ GameInputGamepadRightShoulder, DeviceEnums::InputElementDescriptor::RightShoulder },
	{ GameInputGamepadLeftTriggerButton, DeviceEnums::InputElementDescriptor::LeftTriggerButton },
	{ GameInputGamepadRightTriggerButton, DeviceEnums::InputElementDescriptor::RightTriggerButton },
	{ GameInputGamepadLeftThumbstick, DeviceEnums::InputElementDescriptor::LeftStickButton },
	{ GameInputGamepadRightThumbstick, DeviceEnums::InputElementDescriptor::RightStickButton },
	{ GameInputGamepadPaddleLeft1, DeviceEnums::InputElementDescriptor::PaddleLeft1 },
	{ GameInputGamepadPaddleLeft2, DeviceEnums::InputElementDescriptor::PaddleLeft2 },
	{ GameInputGamepadPaddleRight1, DeviceEnums::InputElementDescriptor::PaddleRight1 },
	{ GameInputGamepadPaddleRight2, DeviceEnums::InputElementDescriptor::PaddleRight2 },
	{ GameInputGamepadDPadDown, DeviceEnums::InputElementDescriptor::DPadDown },
	{ GameInputGamepadDPadUp, DeviceEnums::InputElementDescriptor::DPadUp },
	{ GameInputGamepadDPadLeft, DeviceEnums::InputElementDescriptor::DPadLeft },
	{ GameInputGamepadDPadRight, DeviceEnums::InputElementDescriptor::DPadRight }
};

// The fixed set of axes the gamepad view exposes. Holding the member pointer here means the
// publication order and the per-reading sampling come from the same table.
struct GamepadAxisMapping
{
	DeviceEnums::InputElementDescriptor element;
	float GameInputGamepadState::* field;
};

constexpr GamepadAxisMapping GAMEPAD_AXES[] = {
	{ DeviceEnums::InputElementDescriptor::LeftStickX, &GameInputGamepadState::leftThumbstickX },
	{ DeviceEnums::InputElementDescriptor::LeftStickY, &GameInputGamepadState::leftThumbstickY },
	{ DeviceEnums::InputElementDescriptor::RightStickX, &GameInputGamepadState::rightThumbstickX },
	{ DeviceEnums::InputElementDescriptor::RightStickY, &GameInputGamepadState::rightThumbstickY },
	{ DeviceEnums::InputElementDescriptor::LeftTriggerAxis, &GameInputGamepadState::leftTrigger },
	{ DeviceEnums::InputElementDescriptor::RightTriggerAxis, &GameInputGamepadState::rightTrigger }
};
}

namespace InputMapping
{
// GameInput's neutral label families (letters, numbers, arrows and the remaining icons)
// describe controls identified by their printed glyph rather than by a slot, so they carry
// over to InputElementDescriptor one-to-one. Only ToElement() needs this.
DeviceEnums::InputElementDescriptor ToNeutralElement( GameInputLabel label );

DeviceEnums::InputElementDescriptor ToElement( GameInputLabel label )
{
	using Element = DeviceEnums::InputElementDescriptor;

	switch( label )
	{
	case GameInputLabel::GameInputLabelXboxA:
	case GameInputLabel::GameInputLabelIconCross:
		return Element::FaceSouth;
	case GameInputLabel::GameInputLabelXboxB:
	case GameInputLabel::GameInputLabelIconCircle:
		return Element::FaceEast;
	case GameInputLabel::GameInputLabelXboxX:
	case GameInputLabel::GameInputLabelIconSquare:
		return Element::FaceWest;
	case GameInputLabel::GameInputLabelXboxY:
	case GameInputLabel::GameInputLabelIconTriangle:
		return Element::FaceNorth;

	case GameInputLabel::GameInputLabelXboxLeftShoulder:
	case GameInputLabel::GameInputLabelLB:
	case GameInputLabel::GameInputLabelL1:
		return Element::LeftShoulder;
	case GameInputLabel::GameInputLabelXboxLeftTrigger:
	case GameInputLabel::GameInputLabelLT:
	case GameInputLabel::GameInputLabelL2:
		return Element::LeftTriggerButton;
	case GameInputLabel::GameInputLabelXboxLeftStickButton:
	case GameInputLabel::GameInputLabelLSB:
	case GameInputLabel::GameInputLabelL3:
		return Element::LeftStickButton;

	case GameInputLabel::GameInputLabelXboxRightShoulder:
	case GameInputLabel::GameInputLabelRB:
	case GameInputLabel::GameInputLabelR1:
		return Element::RightShoulder;
	case GameInputLabel::GameInputLabelXboxRightTrigger:
	case GameInputLabel::GameInputLabelRT:
	case GameInputLabel::GameInputLabelR2:
		return Element::RightTriggerButton;
	case GameInputLabel::GameInputLabelXboxRightStickButton:
	case GameInputLabel::GameInputLabelRSB:
	case GameInputLabel::GameInputLabelR3:
		return Element::RightStickButton;

	case GameInputLabel::GameInputLabelXboxMenu:
	case GameInputLabel::GameInputLabelXboxStart:
	case GameInputLabel::GameInputLabelMenu:
	case GameInputLabel::GameInputLabelStart:
	case GameInputLabel::GameInputLabelOptions:
	case GameInputLabel::GameInputLabelIconMenu:
		return Element::Start;

	case GameInputLabel::GameInputLabelXboxView:
	case GameInputLabel::GameInputLabelXboxBack:
	case GameInputLabel::GameInputLabelView:
	case GameInputLabel::GameInputLabelBack:
	case GameInputLabel::GameInputLabelSelect:
	case GameInputLabel::GameInputLabelShare:
		return Element::Select;

	case GameInputLabel::GameInputLabelXboxGuide:
	case GameInputLabel::GameInputLabelGuide:
	case GameInputLabel::GameInputLabelHome:
	case GameInputLabel::GameInputLabelMode:
	case GameInputLabel::GameInputLabelIconHome:
		return Element::Guide;

	case GameInputLabel::GameInputLabelXboxDPadUp:
	case GameInputLabel::GameInputLabelIconDPadUp:
	case GameInputLabel::GameInputLabelUp:
		return Element::DPadUp;
	case GameInputLabel::GameInputLabelXboxDPadDown:
	case GameInputLabel::GameInputLabelIconDPadDown:
	case GameInputLabel::GameInputLabelDown:
		return Element::DPadDown;
	case GameInputLabel::GameInputLabelXboxDPadLeft:
	case GameInputLabel::GameInputLabelIconDPadLeft:
	case GameInputLabel::GameInputLabelLeft:
		return Element::DPadLeft;
	case GameInputLabel::GameInputLabelXboxDPadRight:
	case GameInputLabel::GameInputLabelIconDPadRight:
	case GameInputLabel::GameInputLabelRight:
		return Element::DPadRight;

	case GameInputLabel::GameInputLabelXboxPaddle1:
	case GameInputLabel::GameInputLabelPaddleLeft1:
		return Element::PaddleLeft1;
	case GameInputLabel::GameInputLabelXboxPaddle2:
	case GameInputLabel::GameInputLabelPaddleLeft2:
		return Element::PaddleLeft2;
	case GameInputLabel::GameInputLabelXboxPaddle3:
	case GameInputLabel::GameInputLabelPaddleRight1:
		return Element::PaddleRight1;
	case GameInputLabel::GameInputLabelXboxPaddle4:
	case GameInputLabel::GameInputLabelPaddleRight2:
		return Element::PaddleRight2;

	default:
		// Not a positional element; fall back to the vendor-neutral label families.
		return ToNeutralElement( label );
	}
}

DeviceEnums::InputElementDescriptor ToNeutralElement( GameInputLabel label )
{
	using Element = DeviceEnums::InputElementDescriptor;

	if( label >= GameInputLabel::GameInputLabelLetterA && label <= GameInputLabel::GameInputLabelLetterZ )
	{
		const auto offset = static_cast<uint32_t>( label ) - static_cast<uint32_t>( GameInputLabel::GameInputLabelLetterA );
		return static_cast<Element>( static_cast<uint16_t>( Element::LetterA ) + offset );
	}

	if( label >= GameInputLabel::GameInputLabelNumber0 && label <= GameInputLabel::GameInputLabelNumber9 )
	{
		const auto offset = static_cast<uint32_t>( label ) - static_cast<uint32_t>( GameInputLabel::GameInputLabelNumber0 );
		return static_cast<Element>( static_cast<uint16_t>( Element::Number0 ) + offset );
	}

	switch( label )
	{
	case GameInputLabel::GameInputLabelArrowUp:
		return Element::ArrowUp;
	case GameInputLabel::GameInputLabelArrowUpRight:
		return Element::ArrowUpRight;
	case GameInputLabel::GameInputLabelArrowRight:
		return Element::ArrowRight;
	case GameInputLabel::GameInputLabelArrowDownRight:
		return Element::ArrowDownRight;
	case GameInputLabel::GameInputLabelArrowDown:
		return Element::ArrowDown;
	case GameInputLabel::GameInputLabelArrowDownLLeft:
		return Element::ArrowDownLeft;
	case GameInputLabel::GameInputLabelArrowLeft:
		return Element::ArrowLeft;
	case GameInputLabel::GameInputLabelArrowUpLeft:
		return Element::ArrowUpLeft;
	case GameInputLabel::GameInputLabelArrowUpDown:
		return Element::ArrowUpDown;
	case GameInputLabel::GameInputLabelArrowLeftRight:
		return Element::ArrowLeftRight;
	case GameInputLabel::GameInputLabelArrowUpDownLeftRight:
		return Element::ArrowUpDownLeftRight;
	case GameInputLabel::GameInputLabelArrowClockwise:
		return Element::ArrowClockwise;
	case GameInputLabel::GameInputLabelArrowCounterClockwise:
		return Element::ArrowCounterClockwise;
	case GameInputLabel::GameInputLabelArrowReturn:
		return Element::ArrowReturn;

	case GameInputLabel::GameInputLabelIconBranding:
		return Element::IconBranding;
	case GameInputLabel::GameInputLabelIconStar:
		return Element::IconStar;
	case GameInputLabel::GameInputLabelIconPlus:
		return Element::IconPlus;
	case GameInputLabel::GameInputLabelIconMinus:
		return Element::IconMinus;
	case GameInputLabel::GameInputLabelIconSuspension:
		return Element::IconSuspension;
	case GameInputLabel::GameInputLabelIconDialClockwise:
		return Element::IconDialClockwise;
	case GameInputLabel::GameInputLabelIconDialCounterClockwise:
		return Element::IconDialCounterClockwise;
	case GameInputLabel::GameInputLabelIconSliderLeftRight:
		return Element::IconSliderLeftRight;
	case GameInputLabel::GameInputLabelIconSliderUpDown:
		return Element::IconSliderUpDown;
	case GameInputLabel::GameInputLabelIconWheelUpDown:
		return Element::IconWheelUpDown;

	default:
		return Element::Unknown;
	}
}

DeviceEnums::DeviceFamily GetDeviceFamily( uint16_t vendorId )
{
	constexpr uint16_t VENDOR_MICROSOFT = 0x045E;
	constexpr uint16_t VENDOR_SONY = 0x054C;
	constexpr uint16_t VENDOR_NINTENDO = 0x057E;

	switch( vendorId )
	{
	case VENDOR_MICROSOFT:
		return DeviceEnums::DeviceFamily::Xbox;
	case VENDOR_SONY:
		return DeviceEnums::DeviceFamily::PlayStation;
	case VENDOR_NINTENDO:
		return DeviceEnums::DeviceFamily::Nintendo;
	default:
		return DeviceEnums::DeviceFamily::Generic;
	}
}

BlueSharedString GetDeviceIDAsString( APP_LOCAL_DEVICE_ID deviceId )
{
	std::stringstream ss = {};
	ss << std::hex << std::nouppercase << std::setfill( '0' ) << std::setw( 2 );
	for( size_t i = 0; i < sizeof( deviceId.value ) / sizeof( BYTE ); ++i )
	{
		ss << static_cast<int>( deviceId.value[i] );
	}
	const auto result = ss.str();
	if( result.empty() )
	{
		CCP_LOGERR( "Could not generate device ID for device" );
	}
	return BlueSharedString( result );
}

std::string GetStringValueFromHKLM( const std::string& regSubKey, const std::string& regValue )
{
	size_t bufferSize = 0xFFF; // If too small, will be resized down below.
	std::string valueBuf; // Contiguous buffer since C++11.
	valueBuf.resize( bufferSize );
	auto cbData = static_cast<DWORD>( bufferSize * sizeof( char ) );
	auto rc = RegGetValueA(
		HKEY_LOCAL_MACHINE,
		regSubKey.c_str(),
		regValue.c_str(),
		RRF_RT_REG_SZ,
		nullptr,
		static_cast<void*>( valueBuf.data() ),
		&cbData );
	while( rc == ERROR_MORE_DATA )
	{
		// Get a buffer that is big enough.
		cbData /= sizeof( char );
		if( cbData > static_cast<DWORD>( bufferSize ) )
		{
			bufferSize = static_cast<size_t>( cbData );
		}
		else
		{
			bufferSize *= 2;
			cbData = static_cast<DWORD>( bufferSize * sizeof( char ) );
		}
		valueBuf.resize( bufferSize );
		rc = RegGetValueA(
			HKEY_LOCAL_MACHINE,
			regSubKey.c_str(),
			regValue.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			static_cast<void*>( valueBuf.data() ),
			&cbData );
	}
	if( rc == ERROR_SUCCESS )
	{
		cbData /= sizeof( char );
		valueBuf.resize( static_cast<size_t>( cbData - 1 ) ); // remove end null character
		return valueBuf;
	}
	else
	{
		return std::string( "" );
	}
}

// Events::SwitchPosition is defined to mirror GameInputSwitchPosition value-for-value so the
// conversion is a plain cast. These assertions pin that relationship down: if either enum is
// ever reordered the build breaks here instead of silently reporting wrong directions.
static_assert( static_cast<uint32_t>( Events::SwitchPosition::Center ) == GameInputSwitchCenter, "SwitchPosition::Center must match GameInputSwitchCenter" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::Up ) == GameInputSwitchUp, "SwitchPosition::Up must match GameInputSwitchUp" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::UpRight ) == GameInputSwitchUpRight, "SwitchPosition::UpRight must match GameInputSwitchUpRight" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::Right ) == GameInputSwitchRight, "SwitchPosition::Right must match GameInputSwitchRight" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::DownRight ) == GameInputSwitchDownRight, "SwitchPosition::DownRight must match GameInputSwitchDownRight" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::Down ) == GameInputSwitchDown, "SwitchPosition::Down must match GameInputSwitchDown" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::DownLeft ) == GameInputSwitchDownLeft, "SwitchPosition::DownLeft must match GameInputSwitchDownLeft" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::Left ) == GameInputSwitchLeft, "SwitchPosition::Left must match GameInputSwitchLeft" );
static_assert( static_cast<uint32_t>( Events::SwitchPosition::UpLeft ) == GameInputSwitchUpLeft, "SwitchPosition::UpLeft must match GameInputSwitchUpLeft" );

}

namespace ButtonHandling
{
std::vector<ButtonSource> GetButtonSources( const GameInputControllerInfo* info, const GameInputGamepadInfo* gamepadInfo )
{
	std::vector<ButtonSource> buttonSources;
	if( !info )
	{
		return buttonSources;
	}
	auto totalCount = info->controllerButtonCount;
	auto startIndex = 0;
	buttonSources.reserve( totalCount );

	// Named buttons are identified by descriptor alone; only Unknown buttons are numbered,
	// counted across both views so the sequence matches the published element list.
	uint32_t unknownCount = 0;

	if( gamepadInfo )
	{
		for( const auto& mapping : GAMEPAD_BUTTONS )
		{
			if( ( gamepadInfo->supportedLayout & mapping.mask ) == 0 )
			{
				continue;
			}
			ButtonSource source = {};
			source.kind = ButtonSource::Kind::GamepadMask;
			source.mask = mapping.mask;
			source.key = DeviceEnums::MakeElementKey( mapping.element, unknownCount );
			buttonSources.push_back( source );
		}

		startIndex = totalCount - gamepadInfo->extraButtonCount;
	}

	for( uint32_t rawIndex = startIndex; rawIndex < totalCount; ++rawIndex )
	{
		const auto descriptor = InputMapping::ToElement( info->controllerButtonLabels[rawIndex] );

		// extraButtonCount only says how many extras exist, not where they sit in the raw
		// array. When the tail still carries a control the gamepad view already covers,
		// publishing it again would emit the same element twice.
		if( descriptor != DeviceEnums::InputElementDescriptor::Unknown &&
			std::any_of( buttonSources.begin(), buttonSources.end(), [descriptor]( const ButtonSource& existing ) {
				return existing.key.descriptor == descriptor;
			} ) )
		{
			continue;
		}

		ButtonSource source = {};
		source.kind = ButtonSource::Kind::RawIndex;
		source.rawIndex = rawIndex;
		// Keyed after the skip so a dropped duplicate does not consume an index.
		source.key = DeviceEnums::MakeElementKey( descriptor, unknownCount );
		buttonSources.push_back( source );
	}
	return buttonSources;
}

Events::Button Handle( const ButtonSource& source, const GameInputGamepadState& gamepadState, const bool* rawButtons, uint32_t buttonCount )
{
	Events::Button button;
	switch( source.kind )
	{
	case ButtonSource::Kind::GamepadMask:
		button.pressed = ( gamepadState.buttons & source.mask ) != 0;
		break;
	case ButtonSource::Kind::RawIndex:
		if( source.rawIndex < buttonCount )
		{
			button.pressed = rawButtons[source.rawIndex];
		}
		break;
	default:
		break;
	}
	return button;
}
}

namespace AxisHandling
{
std::vector<AxisSource> GetAxisSources( const GameInputControllerInfo* info, const GameInputGamepadInfo* gamepadInfo )
{
	std::vector<AxisSource> axisSources;
	if( !info )
	{
		return axisSources;
	}
	auto totalCount = info->controllerAxisCount;
	auto startIndex = 0;
	axisSources.reserve( totalCount );

	// Named axes are identified by descriptor alone; only Unknown axes are numbered,
	// counted across both views so the sequence matches the published element list.
	uint32_t unknownCount = 0;

	if( gamepadInfo )
	{
		// Extras have no gamepad-view representation and are only reachable through the raw
		// controller view, at the indices published below.
		for( const auto& mapping : GAMEPAD_AXES )
		{
			AxisSource source = {};
			source.kind = AxisSource::Kind::GamepadField;
			source.gamepadField = mapping.field;
			source.key = DeviceEnums::MakeElementKey( mapping.element, unknownCount );
			axisSources.push_back( source );
		}
		startIndex = totalCount - gamepadInfo->extraAxisCount;
	}
	for( uint32_t rawIndex = startIndex; rawIndex < totalCount; ++rawIndex )
	{
		const auto descriptor = InputMapping::ToElement( info->controllerAxisLabels[rawIndex] );

		// Same reasoning as the buttons: an extra axis that resolves to an element the
		// gamepad view already publishes must not be emitted a second time.
		if( descriptor != DeviceEnums::InputElementDescriptor::Unknown &&
			std::any_of( axisSources.begin(), axisSources.end(), [descriptor]( const AxisSource& existing ) {
				return existing.key.descriptor == descriptor;
			} ) )
		{
			continue;
		}

		AxisSource source = {};
		source.kind = AxisSource::Kind::RawIndex;
		source.rawIndex = rawIndex;
		// Keyed after the skip so a dropped duplicate does not consume an index.
		source.key = DeviceEnums::MakeElementKey( descriptor, unknownCount );
		axisSources.push_back( source );
	}
	return axisSources;
}

Events::Axis Handle( const AxisSource& source, const GameInputGamepadState& gamepadState, const float* rawAxes, uint32_t axisCount )
{
	Events::Axis axis;
	switch( source.kind )
	{
	case AxisSource::Kind::GamepadField:
		if( source.gamepadField )
		{
			axis.value = gamepadState.*source.gamepadField;
		}
		break;
	case AxisSource::Kind::RawIndex:
		if( source.rawIndex < axisCount )
		{
			axis.value = rawAxes[source.rawIndex];
		}
		break;
	}
	return axis;
}
}

namespace SwitchHandling
{
std::vector<SwitchSource> GetSwitchSources( const GameInputControllerInfo* info )
{
	std::vector<SwitchSource> switchSources;
	if( !info )
	{
		return switchSources;
	}
	const auto switchCount = static_cast<uint32_t>( info->controllerSwitchCount );
	switchSources.reserve( switchCount );
	for( uint32_t i = 0; i < switchCount; ++i )
	{
		// Every switch is a d-pad, so the ordinal is what tells them apart.
		switchSources.push_back( { i, { DeviceEnums::InputElementDescriptor::DPad, i } } );
	}
	return switchSources;
}

Events::Switch Handle( const SwitchSource& source, const GameInputSwitchPosition* rawSwitches, uint32_t switchCount )
{
	Events::Switch sw;
	if( source.rawIndex < switchCount )
	{
		const auto position = rawSwitches[source.rawIndex];
		if( position < GameInputSwitchCenter || position > GameInputSwitchUpLeft )
		{
			sw.position = Events::SwitchPosition::Center;
		}
		else
		{
			sw.position = static_cast<Events::SwitchPosition>( position );
		}
	}
	return sw;
}
}

#endif