#ifdef WIN32
#include "InputMappingWin.h"
#include <iomanip>
#include <sstream>


namespace InputMapping
{

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
		cbData /= sizeof( wchar_t );
		if( cbData > static_cast<DWORD>( bufferSize ) )
		{
			bufferSize = static_cast<size_t>( cbData );
		}
		else
		{
			bufferSize *= 2;
			cbData = static_cast<DWORD>( bufferSize * sizeof( wchar_t ) );
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
		cbData /= sizeof( wchar_t );
		valueBuf.resize( static_cast<size_t>( cbData - 1 ) ); // remove end null character
		return valueBuf;
	}
	else
	{
		return std::string( "" );
	}
}

// The subset of GAMEPAD_BUTTONS the device actually exposes, in publication order.
std::vector<GameInputGamepadButtons> GetGamepadButtonMasks( GameInputGamepadButtons supportedLayout )
{
	std::vector<GameInputGamepadButtons> masks;
	masks.reserve( std::size( GAMEPAD_BUTTONS ) );
	for( const auto& mapping : GAMEPAD_BUTTONS )
	{
		if( ( supportedLayout & mapping.mask ) != 0 )
		{
			masks.push_back( mapping.mask );
		}
	}
	return masks;
}

DeviceEnums::InputElementDescriptor GetGamepadButtonDescriptor( GameInputGamepadButtons buttonMask )
{
	for( const auto& mapping : GAMEPAD_BUTTONS )
	{
		if( mapping.mask == buttonMask )
		{
			return mapping.element;
		}
	}
	return DeviceEnums::InputElementDescriptor::Unknown;
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
		const auto masks = InputMapping::GetGamepadButtonMasks( gamepadInfo->supportedLayout );
		for( const auto mask : masks )
		{
			ButtonHandling::ButtonSource source = {};
			source.kind = ButtonHandling::ButtonSource::Kind::GamepadMask;
			source.mask = mask;
			source.descriptor = InputMapping::GetGamepadButtonDescriptor( mask );
			source.elementIndex = Events::AssignElementIndex( source.descriptor, unknownCount );
			buttonSources.push_back( source );
		}

		startIndex = totalCount - gamepadInfo->extraButtonCount;
	}

	for( uint32_t rawIndex = startIndex; rawIndex < totalCount; ++rawIndex )
	{
		ButtonHandling::ButtonSource source = {};
		source.kind = ButtonHandling::ButtonSource::Kind::RawIndex;
		source.rawIndex = rawIndex;
		source.descriptor = InputMapping::ToElement( info->controllerButtonLabels[rawIndex] );

		// extraButtonCount only says how many extras exist, not where they sit in the raw
		// array. When the tail still carries a control the gamepad view already covers,
		// publishing it again would emit the same element twice.
		if( source.descriptor != DeviceEnums::InputElementDescriptor::Unknown &&
			std::any_of( buttonSources.begin(), buttonSources.end(), [&source]( const ButtonSource& existing ) {
				return existing.descriptor == source.descriptor;
			} ) )
		{
			continue;
		}

		// Assigned after the skip so a dropped duplicate does not consume an index.
		source.elementIndex = Events::AssignElementIndex( source.descriptor, unknownCount );
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
		// The gamepad view has a fixed ordering of axes that ReadDeviceState samples by field.
		// Extras have no gamepad-view representation and are only reachable through the raw
		// controller view, at the indices GetAxisIdentifiers published.

		// if we are using a gamepad, then we will get 6 axis (left stick x/y, right stick x/y, left trigger, right trigger)
		for( uint32_t i = 0; i < 6; ++i )
		{
			AxisHandling::AxisSource source = {};
			source.kind = AxisHandling::AxisSource::Kind::GamepadField;
			switch( i )
			{
			case 0:
				source.descriptor = DeviceEnums::InputElementDescriptor::LeftStickX;
				break;
			case 1:
				source.descriptor = DeviceEnums::InputElementDescriptor::LeftStickY;
				break;
			case 2:
				source.descriptor = DeviceEnums::InputElementDescriptor::RightStickX;
				break;
			case 3:
				source.descriptor = DeviceEnums::InputElementDescriptor::RightStickY;
				break;
			case 4:
				source.descriptor = DeviceEnums::InputElementDescriptor::LeftTriggerAxis;
				break;
			case 5:
				source.descriptor = DeviceEnums::InputElementDescriptor::RightTriggerAxis;
				break;
			default:
				break;
			}
			source.rawIndex = i;
			source.elementIndex = Events::AssignElementIndex( source.descriptor, unknownCount );
			axisSources.push_back( source );
		}
		startIndex = totalCount - gamepadInfo->extraAxisCount;
	}
	for( uint32_t rawIndex = startIndex; rawIndex < totalCount; ++rawIndex )
	{
		AxisHandling::AxisSource source = {};
		source.kind = AxisHandling::AxisSource::Kind::RawIndex;
		source.rawIndex = rawIndex;
		source.descriptor = InputMapping::ToElement( info->controllerAxisLabels[rawIndex] );

		// Same reasoning as the buttons: an extra axis that resolves to an element the
		// gamepad view already publishes must not be emitted a second time.
		if( source.descriptor != DeviceEnums::InputElementDescriptor::Unknown &&
			std::any_of( axisSources.begin(), axisSources.end(), [&source]( const AxisSource& existing ) {
				return existing.descriptor == source.descriptor;
			} ) )
		{
			continue;
		}

		// Assigned after the skip so a dropped duplicate does not consume an index.
		source.elementIndex = Events::AssignElementIndex( source.descriptor, unknownCount );
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
		switch( source.descriptor )
		{
		case DeviceEnums::InputElementDescriptor::LeftStickX:
			axis.value = gamepadState.leftThumbstickX;
			break;
		case DeviceEnums::InputElementDescriptor::LeftStickY:
			axis.value = gamepadState.leftThumbstickY;
			break;
		case DeviceEnums::InputElementDescriptor::RightStickX:
			axis.value = gamepadState.rightThumbstickX;
			break;
		case DeviceEnums::InputElementDescriptor::RightStickY:
			axis.value = gamepadState.rightThumbstickY;
			break;
		case DeviceEnums::InputElementDescriptor::LeftTriggerAxis:
			axis.value = gamepadState.leftTrigger;
			break;
		case DeviceEnums::InputElementDescriptor::RightTriggerAxis:
			axis.value = gamepadState.rightTrigger;
			break;
		default:
			break;
		}
		break;
	case AxisSource::Kind::RawIndex:
		if( source.rawIndex < axisCount )
		{
			axis.value = rawAxes[source.rawIndex];
		}
		break;
	default:
		break;
	}
	return axis;
}
}

namespace SwitchHandling
{
std::vector<uint32_t> GetSwitchSources( const GameInputControllerInfo* info )
{
	std::vector<uint32_t> switchSources;
	switchSources.reserve( info->controllerSwitchCount );
	for( uint32_t i = 0; i < static_cast<uint32_t>( info->controllerSwitchCount ); ++i )
	{
		switchSources.push_back( i );
	}
	return switchSources;
}

Events::Switch Handle( const uint32_t& sourceIndex, const GameInputSwitchPosition* rawSwitches, uint32_t switchCount )
{
	Events::Switch sw;
	if( sourceIndex < switchCount )
	{
		const auto position = rawSwitches[sourceIndex];
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