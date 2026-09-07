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
		return Element::LeftTrigger;
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
		return Element::RightTrigger;
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
//
// The D-pad bits are deliberately absent: the D-pad is published as a switch, so listing
// it here as well would duplicate the element. The thumbstick direction bits are absent
// for the same reason - they are derived from the stick axes.
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
	{ GameInputGamepadLeftTriggerButton, DeviceEnums::InputElementDescriptor::LeftTrigger },
	{ GameInputGamepadRightTriggerButton, DeviceEnums::InputElementDescriptor::RightTrigger },
	{ GameInputGamepadLeftThumbstick, DeviceEnums::InputElementDescriptor::LeftStickButton },
	{ GameInputGamepadRightThumbstick, DeviceEnums::InputElementDescriptor::RightStickButton },
	{ GameInputGamepadPaddleLeft1, DeviceEnums::InputElementDescriptor::PaddleLeft1 },
	{ GameInputGamepadPaddleLeft2, DeviceEnums::InputElementDescriptor::PaddleLeft2 },
	{ GameInputGamepadPaddleRight1, DeviceEnums::InputElementDescriptor::PaddleRight1 },
	{ GameInputGamepadPaddleRight2, DeviceEnums::InputElementDescriptor::PaddleRight2 } };

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

// Appends the vendor-specific extras a gamepad-view device declares through
// GameInputGamepadInfo::extraButtonCount / extraAxisCount.
//
// The extras have no gamepad-view representation, so they are only reachable through the
// raw controller view, where they follow the standard layout and therefore occupy the
// trailing entries of the raw arrays.
//
// Raw labels are unreliable on gamepad-view devices and regularly repeat a label the
// standard layout already covers (an extra reporting LSB/RSB, for example). Such an extra
// is not a separate control, so it is dropped entirely rather than published a second
// time. The raw indices of the extras that survive are reported back so ReadDeviceState
// can sample exactly those, keeping state indices aligned with the published elements.
void AppendExtraElements(
	const GameInputLabel* labels,
	uint32_t totalCount,
	uint32_t extraCount,
	std::vector<DeviceEnums::InputElementDescriptor>& elements,
	std::vector<uint32_t>* extraIndices )
{
	if( extraCount == 0 || extraCount > totalCount )
	{
		return;
	}

	const uint32_t firstExtra = totalCount - extraCount;
	for( uint32_t i = 0; i < extraCount; ++i )
	{
		const uint32_t rawIndex = firstExtra + i;
		const auto element = labels ? ToElement( labels[rawIndex] ) : DeviceEnums::InputElementDescriptor::Unknown;
		if( element != DeviceEnums::InputElementDescriptor::Unknown &&
			std::find( elements.begin(), elements.end(), element ) != elements.end() )
		{
			continue;
		}

		elements.push_back( element );
		if( extraIndices )
		{
			extraIndices->push_back( rawIndex );
		}
	}
}

void GetButtonIdentifiers(
	const GameInputControllerInfo* info,
	const GameInputGamepadInfo* gamepadInfo,
	std::vector<DeviceEnums::InputElementDescriptor>& buttonElements,
	std::vector<uint32_t>* extraButtonIndices = nullptr )
{
	buttonElements.clear();
	if( extraButtonIndices )
	{
		extraButtonIndices->clear();
	}

	// Same reasoning as GetAxisIdentifiers: when the device exposes a gamepad view the raw
	// controller buttons carry no trustworthy labels (a DualSense reports GameInputLabelNone
	// for most of them), so the gamepad view is the sole source of button identity when it
	// is available.
	if( gamepadInfo )
	{
		for( const auto& mapping : GAMEPAD_BUTTONS )
		{
			if( ( gamepadInfo->supportedLayout & mapping.mask ) == 0 )
			{
				continue;
			}

			// Identity is the slot itself; the glyph the driver reports is irrelevant and
			// is re-derived from the device family at display time.
			buttonElements.push_back( mapping.element );
		}

		AppendExtraElements(
			info ? info->controllerButtonLabels : nullptr,
			info ? info->controllerButtonCount : 0,
			gamepadInfo->extraButtonCount,
			buttonElements,
			extraButtonIndices );
		return;
	}

	if( !info )
	{
		return;
	}

	const GameInputLabel* labels = info->controllerButtonLabels;
	if( !labels )
	{
		return;
	}
	const uint32_t buttonCount = info->controllerButtonCount;
	buttonElements.reserve( buttonCount );

	for( uint32_t i = 0; i < buttonCount; ++i )
	{
		buttonElements.push_back( ToElement( labels[i] ) );
	}
}

void GetAxisIdentifiers(
	const GameInputControllerInfo* info,
	const GameInputGamepadInfo* gamepadInfo,
	std::vector<DeviceEnums::InputElementDescriptor>& axisElements,
	std::vector<uint32_t>* extraAxisIndices = nullptr )
{
	axisElements.clear();
	if( extraAxisIndices )
	{
		extraAxisIndices->clear();
	}

	// Gamepad-capable devices are described by the gamepad view rather than by the raw
	// controller axes. GameInput makes no guarantee that the raw axis ordering matches the
	// GameInputGamepadState field ordering - on a DualSense it does not - so inferring roles
	// from raw axis indices produces both unmapped and duplicated axes. Emit the documented
	// GameInputGamepadState ordering instead; ReadDeviceState fills these by field.
	if( gamepadInfo )
	{
		axisElements = {
			DeviceEnums::InputElementDescriptor::LeftStickX,
			DeviceEnums::InputElementDescriptor::LeftStickY,
			DeviceEnums::InputElementDescriptor::RightStickX,
			DeviceEnums::InputElementDescriptor::RightStickY,
			DeviceEnums::InputElementDescriptor::LeftTriggerAxis,
			DeviceEnums::InputElementDescriptor::RightTriggerAxis };

		AppendExtraElements(
			info ? info->controllerAxisLabels : nullptr,
			info ? info->controllerAxisCount : 0,
			gamepadInfo->extraAxisCount,
			axisElements,
			extraAxisIndices );
		return;
	}

	if( !info )
	{
		return;
	}
	const GameInputLabel* labels = info->controllerAxisLabels;
	if( !labels )
	{
		return;
	}

	const uint32_t axisCount = info->controllerAxisCount;
	axisElements.reserve( axisCount );
	for( uint32_t i = 0; i < axisCount; ++i )
	{
		// Without a gamepad view the only trustworthy source of meaning is the label itself.
		// A trigger label in the axis dimension denotes the analog travel, not the digital
		// button, so it is remapped to the corresponding axis element.
		auto element = ToElement( labels[i] );
		if( element == DeviceEnums::InputElementDescriptor::LeftTrigger )
		{
			element = DeviceEnums::InputElementDescriptor::LeftTriggerAxis;
		}
		else if( element == DeviceEnums::InputElementDescriptor::RightTrigger )
		{
			element = DeviceEnums::InputElementDescriptor::RightTriggerAxis;
		}
		axisElements.push_back( element );
	}
}

void GetSwitchIdentifiers(
	const GameInputControllerInfo* info,
	std::vector<DeviceEnums::InputElementDescriptor>& switchElements )
{
	switchElements.clear();
	if( !info )
	{
		return;
	}

	switchElements.assign( info->controllerSwitchCount, DeviceEnums::InputElementDescriptor::DPad );
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

Events::SwitchPosition MapSwitchPosition( GameInputSwitchPosition position )
{
	if( position < GameInputSwitchCenter || position > GameInputSwitchUpLeft )
	{
		return Events::SwitchPosition::Center;
	}
	return static_cast<Events::SwitchPosition>( position );
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
			m_gameInput->UnregisterCallback( slot.readCallbackToken );
			slot.device = nullptr;
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
		device->AddRef();
		auto slot = self->GetDeviceSlot( identifier.deviceID );
		if( slot )
		{
			// This can happen if a device disconnects and reconnects again, no need to create a new slot for it, just update the existing one
			if( !slot->device )
			{
				slot->device = device;
			}
			slot->pendingRemoval = false;
			ConfigureGamepadSlot( *slot, device );
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' reconnected", identifier.name.c_str() );
		}
		else
		{
			DeviceSlot slot = {
				device,
				false,
				identifier
			};
			ConfigureGamepadSlot( slot, device );

			std::unique_lock<std::shared_mutex> lock( self->m_deviceMutex );
			self->m_deviceSlots.push_back( std::move( slot ) );
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

	// find the device id
	IGameInputDevice* device;
	reading->GetDevice( &device );

	if( !device )
	{
		return;
	}
	// Resolve the slot first: ReadDeviceState needs its gamepad capability and axis roles.
	// GetDeviceSlot takes m_deviceMutex, so it must not be called while that lock is held.
	auto foundSlot = self->GetDeviceSlot( device );
	if( !foundSlot )
	{
		reading->Release();
		return;
	}

	auto state = self->ReadDeviceState( reading, *foundSlot );
	{
		std::unique_lock<std::shared_mutex> lock( self->m_readingMutex );
		foundSlot->accumulatedStates.push_back( std::move( state ) );
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
			if( slot.pendingRemoval )
			{
				if( slot.device )
				{
					slot.device = nullptr;
					slot.pendingRemoval = false;
					if( slot.readCallbackToken != 0 )
					{
						m_gameInput->UnregisterCallback( slot.readCallbackToken );
						slot.readCallbackToken = 0;
					}
					CCP_LOGNOTICE( "InputHandlerWin: Device '%s' final removal", slot.identifier.name.c_str() );
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
	// The gamepad view, when present, describes buttons and axes; the controller view is
	// still the source for switches and for devices without a gamepad view.
	const bool supportsGamepad = ( info->supportedInput & GameInputKind::GameInputKindGamepad ) != 0;
	const GameInputGamepadInfo* gamepadInfo = supportsGamepad ? info->gamepadInfo : nullptr;

	Mapping::GetButtonIdentifiers( info->controllerInfo, gamepadInfo, identifier.buttonElements );
	Mapping::GetAxisIdentifiers( info->controllerInfo, gamepadInfo, identifier.axisElements );

	if( info->controllerInfo != nullptr )
	{
		Mapping::GetSwitchIdentifiers( info->controllerInfo, identifier.switchElements );
	}

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

std::vector<GameInputGamepadButtons> InputHandlerWin::GetGamepadButtonMasks( IGameInputDevice* device )
{
	if( !device )
	{
		return {};
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr || info->gamepadInfo == nullptr )
	{
		return {};
	}

	return Mapping::GetGamepadButtonMasks( info->gamepadInfo->supportedLayout );
}

void InputHandlerWin::ConfigureGamepadSlot( DeviceSlot& slot, IGameInputDevice* device )
{
	slot.supportsGamepad = SupportsGamepad( device );
	slot.gamepadButtonMasks = GetGamepadButtonMasks( device );
	slot.extraButtonIndices.clear();
	slot.extraAxisIndices.clear();

	if( !slot.supportsGamepad )
	{
		return;
	}

	const GameInputDeviceInfo* info = nullptr;
	device->GetDeviceInfo( &info );
	if( info == nullptr || info->gamepadInfo == nullptr )
	{
		return;
	}

	// Re-run the identifier builders purely for their extra-index output so the sampled raw
	// indices are exactly the ones whose elements were published.
	std::vector<DeviceEnums::InputElementDescriptor> elements;
	Mapping::GetButtonIdentifiers( info->controllerInfo, info->gamepadInfo, elements, &slot.extraButtonIndices );
	Mapping::GetAxisIdentifiers( info->controllerInfo, info->gamepadInfo, elements, &slot.extraAxisIndices );
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
// ReadDeviceState  â€“  get the most recent reading for a device
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::ReadDeviceState( IGameInputReading* reading, const DeviceSlot& slot )
{
	Events::State state = {};

	if( !reading )
	{
		return state;
	}

	state.timestamp = Events::GetTimestamp();

	// convert the GameInputReading into our internal State representation
	auto buttonCount = reading->GetControllerButtonCount();
	auto axisCount = reading->GetControllerAxisCount();
	auto switchCount = reading->GetControllerSwitchCount();

	GameInputGamepadState gamepadState = {};
	const bool hasGamepadState = slot.supportsGamepad && reading->GetGamepadState( &gamepadState );

	// Readings from the generic controller views are needed for extra buttons
	// if the device exposes a gamepad view and has "extra buttons"
	// and for all buttons if it does not.
	auto genericButtonReading = std::make_unique<bool[]>( buttonCount );
	if( buttonCount > 0 && ( !hasGamepadState || !slot.extraButtonIndices.empty() ) )
	{
		reading->GetControllerButtonState( buttonCount, genericButtonReading.get() );
	}

	// Same for axes
	auto genericAxisReading = std::make_unique<float[]>( axisCount );
	if( axisCount > 0 && ( !hasGamepadState || !slot.extraAxisIndices.empty() ) )
	{
		reading->GetControllerAxisState( axisCount, genericAxisReading.get() );
	}


	if( hasGamepadState )
	{
		// Matches the elements GetButtonIdentifiers published for this device: the buttons
		// GameInputGamepadInfo::supportedLayout advertises, in the same order.
		state.buttons.resize( slot.gamepadButtonMasks.size() );
		for( size_t index = 0; index < slot.gamepadButtonMasks.size(); ++index )
		{
			state.buttons[index].descriptor = slot.identifier.buttonElements[index];
			state.buttons[index].index = static_cast<uint32_t>(index);
			state.buttons[index].pressed = ( gamepadState.buttons & slot.gamepadButtonMasks[index] ) != 0;
		}

		// The extras have no gamepad-view representation; they live in the raw controller view
		// at the indices GetButtonIdentifiers actually published.
		if( !slot.extraButtonIndices.empty() && buttonCount > 0 )
		{
			for( uint32_t rawIndex : slot.extraButtonIndices )
			{
				Events::Button extra = {};
				extra.descriptor = slot.identifier.buttonElements[rawIndex];
				extra.index = rawIndex;
				extra.pressed = rawIndex < buttonCount && genericButtonReading[rawIndex] != 0;
				state.buttons.push_back( extra );
			}
		}
	}
	else if( buttonCount > 0 )
	{
		state.buttons.resize( buttonCount );
		for( uint32_t index = 0; index < buttonCount; ++index )
		{
			state.buttons[index].descriptor = slot.identifier.buttonElements[index];
			state.buttons[index].index = static_cast<uint32_t>( index );
			state.buttons[index].pressed = genericButtonReading[index] != 0;
		}
	}

	// Raw controller axes are normalized to the HID logical range, so a centered stick
	// reads 0.5 rather than 0.0, and the raw ordering does not necessarily match the
	// gamepad field ordering. The gamepad view uses the documented GameInputGamepadState
	// convention (-1..1 for sticks with right/up positive, 0..1 for triggers), so when the
	// device exposes one it is the sole source of axis data and the order matches the
	// six elements GetAxisIdentifiers published for this device.
	if( hasGamepadState )
	{
		state.axis.resize( axisCount );

		for( uint32_t index = 0; index < axisCount; ++index )
		{
			Events::Axis axis = {};
			auto descriptor = slot.identifier.axisElements[index];
			axis.descriptor = descriptor;
			axis.index = index;
			switch( descriptor )
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
			case DeviceEnums::InputElementDescriptor::Unknown:
				axis.value = genericAxisReading[index];
				break;
			default:
				CCP_LOGERR( "InputHandlerWin: Unexpected axis descriptor %d for device '%s'", DeviceEnums::ToKeyString( descriptor ), slot.identifier.name.c_str() );
				break;
			}
			state.axis[index] = axis;
		}
	}
	else if( axisCount > 0 )
	{
		state.axis.resize( axisCount );
		for( uint32_t index = 0; index < axisCount; ++index )
		{
			Events::Axis axis = {};
			auto descriptor = slot.identifier.axisElements[index];
			axis.descriptor = descriptor;
			axis.index = index;
			axis.value = genericAxisReading[index];
			state.axis[index] = axis;
		}
	}

	if( switchCount > 0 )
	{
		auto switchReading = std::make_unique<GameInputSwitchPosition[]>( switchCount );
		state.switches.resize( switchCount );
		reading->GetControllerSwitchState( switchCount, switchReading.get() );
		for( uint32_t index = 0; index < state.switches.size(); ++index )
		{
			Events::Switch sw = {};
			sw.descriptor = slot.identifier.switchElements[index];
			sw.index = index;
			sw.position = Mapping::MapSwitchPosition( switchReading[index] );
			state.switches[index] = sw;
		}
	}

	reading->Release();

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

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [deviceID]( const DeviceSlot& slot ) {
		return slot.device && slot.identifier.deviceID == deviceID;
	} );
	if( it != m_deviceSlots.end() )
	{
		return &( *it );
	}

	return nullptr;
}

InputHandlerWin::DeviceSlot* InputHandlerWin::GetDeviceSlot( CComPtr<IGameInputDevice> device )
{
	std::unique_lock<std::shared_mutex> lock( m_deviceMutex );

	auto it = std::find_if( m_deviceSlots.begin(), m_deviceSlots.end(), [device]( const DeviceSlot& slot ) {
		return slot.device == device;
	} );
	if( it != m_deviceSlots.end() )
	{
		return &( *it );
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
