#ifdef WIN32
#include "InputHandlerWin.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <Windows.h> 
#include <gameinput_v3.h>
#include <iomanip>

#include "../ControlManager.h"

using namespace GameInput::v3;

namespace RegistryValues
{
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
}

namespace Mapping
{
DeviceEnums::InputElementDescriptor ToNeutralElement( GameInputLabel label );

// Folds the several parallel GameInputLabel naming families (Xbox letters, PlayStation
// icons, generic words, LB/L1 style abbreviations) down to the abstract slot the element
// occupies. The glyph flavour the driver reported is deliberately discarded: identity is
// positional, and the flavour is re-applied at display time from the device family.
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

// GameInput's neutral label families (letters, numbers, arrows and the remaining icons)
// describe controls that are identified by their printed glyph rather than by a slot, so
// they carry over to InputElement one-to-one.
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



// Hardware families are resolved from the USB vendor ID rather than from the labels the
// driver reports, so the same controller yields the same glyph flavour regardless of how
// the OS chose to describe it.
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

// Canonical ordering of the gamepad view's buttons.
//
// GameInputGamepadInfo::supportedLayout advertises exactly which of these a device
// actually has - even a generic pad reports a layout - so the published button list is
// built from that mask instead of a hard-coded set. Entries are emitted in this order and
// ReadDeviceState samples the same order, keeping indices aligned.
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
		const auto masks = Mapping::GetGamepadButtonMasks( gamepadInfo->supportedLayout );
		for( const auto mask : masks )
		{
			ButtonHandling::ButtonSource source = {};
			source.kind = ButtonHandling::ButtonSource::Kind::GamepadMask;
			source.mask = mask;
			source.descriptor = Mapping::GetGamepadButtonDescriptor( mask );
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
		source.descriptor = Mapping::ToElement( info->controllerButtonLabels[rawIndex] );

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
	button.descriptor = source.descriptor;
	button.index = source.elementIndex;
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
			axisSources.push_back( source );
		}
		startIndex = totalCount - gamepadInfo->extraAxisCount;
	}
	for( uint32_t rawIndex = startIndex; rawIndex < totalCount; ++rawIndex )
	{
		AxisHandling::AxisSource source = {};
		source.kind = AxisHandling::AxisSource::Kind::RawIndex;
		source.rawIndex = rawIndex;
		source.descriptor = Mapping::ToElement( info->controllerAxisLabels[rawIndex] );

		// Same reasoning as the buttons: an extra axis that resolves to an element the
		// gamepad view already publishes must not be emitted a second time.
		if( source.descriptor != DeviceEnums::InputElementDescriptor::Unknown &&
			std::any_of( axisSources.begin(), axisSources.end(), [&source]( const AxisSource& existing ) {
				return existing.descriptor == source.descriptor;
			} ) )
		{
			continue;
		}

		axisSources.push_back( source );
	}
	return axisSources;
}

Events::Axis Handle( const AxisSource& source, const GameInputGamepadState& gamepadState, const float* rawAxes, uint32_t axisCount )
{
	Events::Axis axis;
	axis.descriptor = source.descriptor;
	axis.index = source.rawIndex;
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
	sw.index = sourceIndex;
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

namespace
{
// Converts the 32 byte device ID from GameInput into a string to be used as a unique identifier for devices.BlueSharedString GetDeviceIDAsString( APP_LOCAL_DEVICE_ID deviceId )
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
}

InputHandlerWin::InputHandlerWin()
{
}

InputHandlerWin::~InputHandlerWin()
{
	if( !m_initialized )
	{
		return;
	}

	// Unregister the device callback before releasing devices
	if( m_gameInput )
	{
		if( m_deviceCallbackToken != 0 )
		{
			m_gameInput->UnregisterCallback( m_deviceCallbackToken );
			m_deviceCallbackToken = 0;
		}
	}

	{
		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			m_gameInput->UnregisterCallback( slot->readCallbackToken );
			slot->device = nullptr;
		}
	}

	if( m_gameInput )
	{
		m_gameInput = nullptr;
	}

	CCP_LOGNOTICE( "InputHandlerWin: Shut down" );
}


// ---------------------------------------------------------------------------
// GameInput lifetime
// ---------------------------------------------------------------------------
bool InputHandlerWin::Initialize()
{
	if( m_initialized )
	{
		return true;
	}

	HRESULT hr = GameInputCreate( &m_gameInput );
	if( FAILED( hr ) || !m_gameInput )
	{
		CCP_LOGERR( "InputHandlerWin: GameInputCreate failed (0x%08X)", hr );
		return false;
	}

	// Register for device connect / disconnect notifications.
	// Passing nullptr for the device filter means we get notified for all devices.
	hr = m_gameInput->RegisterDeviceCallback(
		nullptr, // no specific device filter
		SUPPORTED_INPUTS,
		GameInputDeviceConnected, // status filter
		GameInputBlockingEnumeration, // enumerate already-connected devices synchronously
		this, // context
		OnDeviceStatusChanged,
		&m_deviceCallbackToken );

	if( FAILED( hr ) )
	{
		CCP_LOGERR( "InputHandlerWin: RegisterDeviceCallback failed (0x%08X)", hr );
		m_gameInput = nullptr;
		return false;
	}

	m_initialized = true;
	CCP_LOGNOTICE( "InputHandlerWin: Initialized successfully" );
	return true;
}

// ---------------------------------------------------------------------------
// Device callback  (may be called from any thread)
// ---------------------------------------------------------------------------
void CALLBACK InputHandlerWin::OnDeviceStatusChanged(
	_In_ GameInputCallbackToken,
	_In_ void* context,
	_In_ IGameInputDevice* device,
	_In_ uint64_t,
	_In_ GameInputDeviceStatus currentStatus,
	_In_ GameInputDeviceStatus previousStatus ) noexcept
{
	auto* self = reinterpret_cast<InputHandlerWin*>( context );
	if( !self )
	{
		return;
	}

	const bool wasConnected = ( previousStatus & GameInputDeviceConnected ) != 0;
	const bool isConnected = ( currentStatus & GameInputDeviceConnected ) != 0;

	auto identifier = self->GetIdentifier( device );
	if( isConnected && !wasConnected )
	{
		auto slot = self->GetDeviceSlot( identifier.deviceID );
		if( slot )
		{
			// This can happen if a device disconnects and reconnects again, no need to create a new slot for it, just update the existing one
			if( !slot->device )
			{
				slot->device = device;
			}
			slot->pendingRemoval = false;
			ConfigureDeviceSlot( *slot, device );
			identifier = slot->identifier;
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' reconnected", identifier.name.c_str() );
		}
		else
		{
			auto newSlot = std::make_unique<DeviceSlot>();
			newSlot->device = device;
			newSlot->pendingRemoval = false;
			newSlot->identifier = identifier;
			ConfigureDeviceSlot( *newSlot, device );
			// ConfigureDeviceSlot rewrites the element lists to match what will
			// actually be published, so the callback must see the configured copy.
			identifier = newSlot->identifier;

			std::unique_lock<std::shared_mutex> lock( self->m_deviceMutex );
			self->m_deviceSlots.push_back( std::move( newSlot ) );
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' connected", identifier.name.c_str() );
		}
		if( self->m_deviceAddedCallback )
		{
			self->m_deviceAddedCallback( identifier );
		}
	}
	else if( !isConnected && wasConnected )
	{
		// Mark the matching slot for removal on next Update()
		auto slot = self->GetDeviceSlot( identifier.deviceID );
		if( slot )
		{
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final disconnected", slot->identifier.name.c_str() );

			slot->pendingRemoval = true;
			self->m_devicesRemoved = true;
			if( self->m_deviceRemovedCallback )
			{
				self->m_deviceRemovedCallback( slot->identifier );
			}
		}
	}
}

void CALLBACK InputHandlerWin::OnDeviceRead(
	_In_ GameInputCallbackToken callbackToken,
	_In_ void* context,
	_In_ IGameInputReading* reading ) noexcept
{
	auto* self = reinterpret_cast<InputHandlerWin*>( context );
	if( !self || !reading )
	{
		return;
	}

	// Take ownership of the reading reference so it is released on every exit path
	CComPtr<IGameInputReading> ownedReading;
	ownedReading.Attach( reading );

	// find the device id. GetDevice hands back a reference, so it needs releasing too.
	CComPtr<IGameInputDevice> device;
	ownedReading->GetDevice( &device );

	if( !device )
	{
		return;
	}
	// Resolve the slot first: ReadDeviceState needs its gamepad capability and axis roles.
	// GetDeviceSlot takes m_deviceMutex, so it must not be called while that lock is held.
	auto foundSlot = self->GetDeviceSlot( device );
	if( !foundSlot )
	{
		return;
	}

	auto state = self->ReadDeviceState( ownedReading, *foundSlot );
	if( !state )
	{
		// An unreadable reading tells us nothing; publishing an empty snapshot would look
		// like every element on the device had just gone neutral.
		return;
	}

	{
		std::unique_lock<std::shared_mutex> lock( self->m_readingMutex );
		foundSlot->accumulatedStates.push_back( std::move( *state ) );
	}
}

void InputHandlerWin::SetDeviceActivation( BlueSharedString deviceID, bool activate )
{
	auto foundDevice = GetDeviceSlot( deviceID );
	if( !foundDevice )
	{
		CCP_LOGERR( "InputHandlerWin: Could not find device with ID '%s' to set activation to %d", deviceID.c_str(), activate );
		return;
	}

	if( activate )
	{
		auto hr = m_gameInput->RegisterReadingCallback(
			nullptr,
			SUPPORTED_INPUTS,
			this,
			OnDeviceRead,
			&foundDevice->readCallbackToken );
		if( !SUCCEEDED( hr ) )
		{
			CCP_LOGERR( "InputHandlerWin: Failed to register reading callback for device '%s' (0x%08X)", foundDevice->identifier.name.c_str(), hr );
			foundDevice->readCallbackToken = 0;
		}
	}
	else
	{
		if( foundDevice->device )
		{
			GameInputRumbleParams zeroed = {};
			std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
			foundDevice->device->SetRumbleState( &zeroed );
		}
		if( foundDevice->readCallbackToken != 0 )
		{
			m_gameInput->UnregisterCallback( foundDevice->readCallbackToken );
			foundDevice->readCallbackToken = 0;
		}
	}
}

std::vector<Events::State> InputHandlerWin::Update( BlueSharedString deviceID )
{
	if( !m_initialized )
	{
		return {};
	}

	// need to remove devices here, but not in the callback
	if( m_devicesRemoved )
	{
		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		for( auto& slot : m_deviceSlots )
		{
			if( slot->pendingRemoval )
			{
				if( slot->device )
				{
					slot->device = nullptr;
					slot->pendingRemoval = false;
					if( slot->readCallbackToken != 0 )
					{
						m_gameInput->UnregisterCallback( slot->readCallbackToken );
						slot->readCallbackToken = 0;
					}
					CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final removal", slot->identifier.name.c_str() );
				}
			}
		}
		m_devicesRemoved = false;
	}

	std::vector<Events::State> statesForDevice = {};
	{
		auto deviceSlot = GetDeviceSlot( deviceID );
		if( !deviceSlot )
		{
			CCP_LOGERR( "InputHandlerWin: Could not find device with ID '%s' to update", deviceID.c_str() );
			return statesForDevice;
		}
		std::unique_lock<std::shared_mutex> lock( m_readingMutex );
		std::swap( statesForDevice, deviceSlot->accumulatedStates );
	}

	return statesForDevice;
}

DeviceEnums::DeviceIdentifier InputHandlerWin::GetIdentifier( IGameInputDevice* device )
{
	DeviceEnums::DeviceIdentifier identifier;

	if( !device )
	{
		return identifier;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr )
	{
		return identifier;
	}

	identifier.deviceID = GetDeviceIDAsString( info->deviceId );

	char vid[16];
	snprintf( vid, sizeof( vid ), "%04X", info->vendorId );

	char pid[16];
	snprintf( pid, sizeof( pid ), "%04X", info->productId );

	identifier.vendorID = BlueSharedString( vid );
	identifier.productID = BlueSharedString( pid );
	identifier.family = Mapping::GetDeviceFamily( info->vendorId );

	if( info->deviceFamily == GameInputDeviceFamily::GameInputFamilyHid )
	{
		// check the registry for the device name, using the vendor/product ID as a key
		auto registryName = RegistryValues::GetStringValueFromHKLM(
			"SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\VID_" + std::string( vid ) + "&PID_" + std::string( pid ),
			"OEMName" );
		if( !registryName.empty() )
		{
			identifier.name = BlueSharedString( registryName.c_str() );
		}
	}

	if( identifier.name.empty() && info->displayName )
	{
		identifier.name = BlueSharedString( static_cast<const char*>( info->displayName ) );
	}

	bool hasLowFreq = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleLowFrequency ) != 0;
	bool hasHighFreq = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleHighFrequency ) != 0;
	bool hasLeftTrigger = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleLeftTrigger ) != 0;
	bool hasRightTrigger = ( info->supportedRumbleMotors & GameInputRumbleMotors::GameInputRumbleRightTrigger ) != 0;

	identifier.rumbleCapacity.hasLowFrequencyRumble = hasLowFreq;
	identifier.rumbleCapacity.hasHighFrequencyRumble = hasHighFreq;
	identifier.rumbleCapacity.hasLeftTriggerRumble = hasLeftTrigger;
	identifier.rumbleCapacity.hasRightTriggerRumble = hasRightTrigger;

	identifier.rumbleCapacity.rumbleMotorCount = hasLowFreq + hasHighFreq + hasLeftTrigger + hasRightTrigger;

	return identifier;
}

bool InputHandlerWin::SupportsGamepad( IGameInputDevice* device )
{
	if( !device )
	{
		return false;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr )
	{
		return false;
	}

	// Without the gamepad info block the layout is unknown, so the gamepad view cannot be
	// used as the source of button identity.
	return ( info->supportedInput & GameInputKind::GameInputKindGamepad ) != 0 && info->gamepadInfo != nullptr;
}

void InputHandlerWin::ConfigureDeviceSlot( DeviceSlot& slot, IGameInputDevice* device )
{
	slot.buttonSources.clear();
	slot.axisSources.clear();
	slot.switchSources.clear();
	// The published element lists are rebuilt below. On a reconnect the slot still carries
	// the previous run's entries, which would otherwise be appended to instead of replaced.
	slot.identifier.buttonElements.clear();
	slot.identifier.axisElements.clear();
	slot.identifier.switchElements.clear();
	slot.needsGamepadState = false;
	slot.needsRawButtons = false;
	slot.needsRawAxes = false;
	slot.needsRawSwitches = false;
	slot.supportsGamepad = SupportsGamepad( device );

	if( !device )
	{
		return;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr )
	{
		return;
	}

	const GameInputGamepadInfo* gamepadInfo = slot.supportsGamepad ? info->gamepadInfo : nullptr;
	const GameInputControllerInfo* controllerInfo = info->controllerInfo;

	// --- Buttons -----------------------------------------------------------
	slot.buttonSources = ButtonHandling::GetButtonSources( controllerInfo, gamepadInfo );

	// --- Axes --------------------------------------------------------------
	slot.axisSources = AxisHandling::GetAxisSources( controllerInfo, gamepadInfo );

	// if we have a gamepad, then the dpad buttons are handled as buttons, not switches. Otherwise, the dpad is handled as a switch.
	if( !slot.supportsGamepad )
	{
		// --- Switches -----------------------------------------------------------
		slot.switchSources = SwitchHandling::GetSwitchSources( controllerInfo );
	}

	// --- Reading requirements ----------------------------------------------
	// Decided once here so ReadDeviceState never has to work out which views to
	// fetch for a given reading.
	for( const auto& source : slot.buttonSources )
	{
		slot.needsGamepadState |= source.kind == ButtonHandling::ButtonSource::Kind::GamepadMask;
		slot.needsRawButtons |= source.kind == ButtonHandling::ButtonSource::Kind::RawIndex;
		slot.identifier.buttonElements.push_back( source.descriptor );
	}
	for( const auto& source : slot.axisSources )
	{
		slot.needsGamepadState |= source.kind == AxisHandling::AxisSource::Kind::GamepadField;
		slot.needsRawAxes |= source.kind == AxisHandling::AxisSource::Kind::RawIndex;
		slot.identifier.axisElements.push_back( source.descriptor );
	}
	for( const auto& sourceIndex : slot.switchSources )
	{
		slot.identifier.switchElements.push_back( DeviceEnums::InputElementDescriptor::DPad );
	}
	slot.needsRawSwitches = !slot.switchSources.empty();
}

void InputHandlerWin::RegisterForDeviceAdded( DeviceChangedCallback callback )
{
	m_deviceAddedCallback = callback;
}

void InputHandlerWin::RegisterForDeviceRemoved( DeviceChangedCallback callback )
{
	m_deviceRemovedCallback = callback;
}

// ---------------------------------------------------------------------------
// ReadDeviceState - get the most recent reading for a device
// ---------------------------------------------------------------------------
std::optional<Events::State> InputHandlerWin::ReadDeviceState( IGameInputReading* reading, const DeviceSlot& slot )
{
	// Pure execution of the slot's extraction plan. Every layout question was
	// answered once in ConfigureDeviceSlot, so nothing here inspects device
	// capabilities or element descriptors.
	if( !reading )
	{
		return std::nullopt;
	}

	Events::State state = {};
	state.timestamp = Events::GetTimestamp();

	// retrieve the needed gamepad and controller information
	GameInputGamepadState gamepadState = {};
	if( slot.needsGamepadState )
	{
		if( !reading->GetGamepadState( &gamepadState ) )
		{
			return std::nullopt;
		}
	}

	const uint32_t buttonCount = slot.needsRawButtons ? reading->GetControllerButtonCount() : 0;
	auto rawButtons = std::make_unique<bool[]>( buttonCount );
	if( slot.needsRawButtons )
	{
		reading->GetControllerButtonState( buttonCount, rawButtons.get() );
	}

	const uint32_t axisCount = slot.needsRawAxes ? reading->GetControllerAxisCount() : 0;
	auto rawAxes = std::make_unique<float[]>( axisCount );
	if( slot.needsRawAxes )
	{
		reading->GetControllerAxisState( axisCount, rawAxes.get() );
	}

	const uint32_t switchCount = slot.needsRawSwitches ? reading->GetControllerSwitchCount() : 0;
	auto rawSwitches = std::make_unique<GameInputSwitchPosition[]>( switchCount );
	if( switchCount > 0 )
	{
		reading->GetControllerSwitchState( switchCount, rawSwitches.get() );
	}

	// handle the buttons
	state.buttons.reserve( slot.buttonSources.size() );
	for( const auto& source : slot.buttonSources )
	{
		state.buttons.push_back( ButtonHandling::Handle( source, gamepadState, rawButtons.get(), buttonCount ) );
	}

	// handle the axes
	state.axis.reserve( slot.axisSources.size() );
	for( const auto& source : slot.axisSources )
	{
		state.axis.push_back( AxisHandling::Handle( source, gamepadState, rawAxes.get(), axisCount ) );
	}

	// handle the switches
	state.switches.reserve( slot.switchSources.size() );
	for( const auto& sourceIndex : slot.switchSources )
	{
		state.switches.push_back( SwitchHandling::Handle( sourceIndex, rawSwitches.get(), switchCount ) );
	}

	return state;
}

void InputHandlerWin::Rumble( BlueSharedString deviceID, Events::Rumble rumble )
{
	auto deviceSlot = GetDeviceSlot( deviceID );

	if( deviceSlot )
	{
		GameInputRumbleParams rumbleParams = {};
		rumbleParams.highFrequency = rumble.highFrequency;
		rumbleParams.lowFrequency = rumble.lowFrequency;
		rumbleParams.leftTrigger = rumble.leftTrigger;
		rumbleParams.rightTrigger = rumble.rightTrigger;

		std::unique_lock<std::shared_mutex> lock( m_deviceMutex );
		deviceSlot->device->SetRumbleState( &rumbleParams );
	}
}

InputHandlerWin::DeviceSlot* InputHandlerWin::GetDeviceSlot( BlueSharedString deviceID )
{
	std::unique_lock<std::shared_mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const std::unique_ptr<DeviceSlot>& slot ) {
		return slot->device && slot->identifier.deviceID == deviceID;
	} );
	if( it != m_deviceSlots.end() )
	{
		return it->get();
	}

	return nullptr;
}

InputHandlerWin::DeviceSlot* InputHandlerWin::GetDeviceSlot( CComPtr<IGameInputDevice> device )
{
	std::unique_lock<std::shared_mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [device]( const std::unique_ptr<DeviceSlot>& slot ) {
		return slot->device == device;
	} );
	if( it != m_deviceSlots.end() )
	{
		return it->get();
	}

	return nullptr;
}

void InputHandlerWin::SetBackgroundEventsEnabled( bool enabled )
{
	if( m_gameInput )
	{
		if( enabled )
		{
			m_gameInput->SetFocusPolicy( GameInputFocusPolicy::GameInputEnableBackgroundInput );
		}
		else
		{
			m_gameInput->SetFocusPolicy( GameInputFocusPolicy::GameInputDefaultFocusPolicy );
		}
	}
}
#endif // WIN32
