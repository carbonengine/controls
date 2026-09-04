#ifdef WIN32
#include "InputHandlerWin.h"

#include <algorithm>
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
BlueSharedString GetName( GameInputLabel label )
{
	switch( label )
	{
	case GameInputLabel::GameInputLabelUnknown:
		return BlueSharedString( "Unknown" );
	case GameInputLabel::GameInputLabelNone:
		return BlueSharedString( "None" );
	case GameInputLabel::GameInputLabelXboxGuide:
		return BlueSharedString( "XboxGuide" );
	case GameInputLabel::GameInputLabelXboxBack:
		return BlueSharedString( "XboxBack" );
	case GameInputLabel::GameInputLabelXboxStart:
		return BlueSharedString( "XboxStart" );
	case GameInputLabel::GameInputLabelXboxMenu:
		return BlueSharedString( "XboxMenu" );
	case GameInputLabel::GameInputLabelXboxView:
		return BlueSharedString( "XboxView" );
	case GameInputLabel::GameInputLabelXboxA:
		return BlueSharedString( "XboxA" );
	case GameInputLabel::GameInputLabelXboxB:
		return BlueSharedString( "XboxB" );
	case GameInputLabel::GameInputLabelXboxX:
		return BlueSharedString( "XboxX" );
	case GameInputLabel::GameInputLabelXboxY:
		return BlueSharedString( "XboxY" );
	case GameInputLabel::GameInputLabelXboxDPadUp:
		return BlueSharedString( "XboxDPadUp" );
	case GameInputLabel::GameInputLabelXboxDPadDown:
		return BlueSharedString( "XboxDPadDown" );
	case GameInputLabel::GameInputLabelXboxDPadLeft:
		return BlueSharedString( "XboxDPadLeft" );
	case GameInputLabel::GameInputLabelXboxDPadRight:
		return BlueSharedString( "XboxDPadRight" );
	case GameInputLabel::GameInputLabelXboxLeftShoulder:
		return BlueSharedString( "XboxLeftShoulder" );
	case GameInputLabel::GameInputLabelXboxLeftTrigger:
		return BlueSharedString( "XboxLeftTrigger" );
	case GameInputLabel::GameInputLabelXboxLeftStickButton:
		return BlueSharedString( "XboxLeftStickButton" );
	case GameInputLabel::GameInputLabelXboxRightShoulder:
		return BlueSharedString( "XboxRightShoulder" );
	case GameInputLabel::GameInputLabelXboxRightTrigger:
		return BlueSharedString( "XboxRightTrigger" );
	case GameInputLabel::GameInputLabelXboxRightStickButton:
		return BlueSharedString( "XboxRightStickButton" );
	case GameInputLabel::GameInputLabelXboxPaddle1:
		return BlueSharedString( "XboxPaddle1" );
	case GameInputLabel::GameInputLabelXboxPaddle2:
		return BlueSharedString( "XboxPaddle2" );
	case GameInputLabel::GameInputLabelXboxPaddle3:
		return BlueSharedString( "XboxPaddle3" );
	case GameInputLabel::GameInputLabelXboxPaddle4:
		return BlueSharedString( "XboxPaddle4" );
	case GameInputLabel::GameInputLabelLetterA:
		return BlueSharedString( "LetterA" );
	case GameInputLabel::GameInputLabelLetterB:
		return BlueSharedString( "LetterB" );
	case GameInputLabel::GameInputLabelLetterC:
		return BlueSharedString( "LetterC" );
	case GameInputLabel::GameInputLabelLetterD:
		return BlueSharedString( "LetterD" );
	case GameInputLabel::GameInputLabelLetterE:
		return BlueSharedString( "LetterE" );
	case GameInputLabel::GameInputLabelLetterF:
		return BlueSharedString( "LetterF" );
	case GameInputLabel::GameInputLabelLetterG:
		return BlueSharedString( "LetterG" );
	case GameInputLabel::GameInputLabelLetterH:
		return BlueSharedString( "LetterH" );
	case GameInputLabel::GameInputLabelLetterI:
		return BlueSharedString( "LetterI" );
	case GameInputLabel::GameInputLabelLetterJ:
		return BlueSharedString( "LetterJ" );
	case GameInputLabel::GameInputLabelLetterK:
		return BlueSharedString( "LetterK" );
	case GameInputLabel::GameInputLabelLetterL:
		return BlueSharedString( "LetterL" );
	case GameInputLabel::GameInputLabelLetterM:
		return BlueSharedString( "LetterM" );
	case GameInputLabel::GameInputLabelLetterN:
		return BlueSharedString( "LetterN" );
	case GameInputLabel::GameInputLabelLetterO:
		return BlueSharedString( "LetterO" );
	case GameInputLabel::GameInputLabelLetterP:
		return BlueSharedString( "LetterP" );
	case GameInputLabel::GameInputLabelLetterQ:
		return BlueSharedString( "LetterQ" );
	case GameInputLabel::GameInputLabelLetterR:
		return BlueSharedString( "LetterR" );
	case GameInputLabel::GameInputLabelLetterS:
		return BlueSharedString( "LetterS" );
	case GameInputLabel::GameInputLabelLetterT:
		return BlueSharedString( "LetterT" );
	case GameInputLabel::GameInputLabelLetterU:
		return BlueSharedString( "LetterU" );
	case GameInputLabel::GameInputLabelLetterV:
		return BlueSharedString( "LetterV" );
	case GameInputLabel::GameInputLabelLetterW:
		return BlueSharedString( "LetterW" );
	case GameInputLabel::GameInputLabelLetterX:
		return BlueSharedString( "LetterX" );
	case GameInputLabel::GameInputLabelLetterY:
		return BlueSharedString( "LetterY" );
	case GameInputLabel::GameInputLabelLetterZ:
		return BlueSharedString( "LetterZ" );
	case GameInputLabel::GameInputLabelNumber0:
		return BlueSharedString( "Number0" );
	case GameInputLabel::GameInputLabelNumber1:
		return BlueSharedString( "Number1" );
	case GameInputLabel::GameInputLabelNumber2:
		return BlueSharedString( "Number2" );
	case GameInputLabel::GameInputLabelNumber3:
		return BlueSharedString( "Number3" );
	case GameInputLabel::GameInputLabelNumber4:
		return BlueSharedString( "Number4" );
	case GameInputLabel::GameInputLabelNumber5:
		return BlueSharedString( "Number5" );
	case GameInputLabel::GameInputLabelNumber6:
		return BlueSharedString( "Number6" );
	case GameInputLabel::GameInputLabelNumber7:
		return BlueSharedString( "Number7" );
	case GameInputLabel::GameInputLabelNumber8:
		return BlueSharedString( "Number8" );
	case GameInputLabel::GameInputLabelNumber9:
		return BlueSharedString( "Number9" );
	case GameInputLabel::GameInputLabelArrowUp:
		return BlueSharedString( "ArrowUp" );
	case GameInputLabel::GameInputLabelArrowUpRight:
		return BlueSharedString( "ArrowUpRight" );
	case GameInputLabel::GameInputLabelArrowRight:
		return BlueSharedString( "ArrowRight" );
	case GameInputLabel::GameInputLabelArrowDownRight:
		return BlueSharedString( "ArrowDownRight" );
	case GameInputLabel::GameInputLabelArrowDown:
		return BlueSharedString( "ArrowDown" );
	case GameInputLabel::GameInputLabelArrowDownLLeft:
		return BlueSharedString( "ArrowDownLeft" );
	case GameInputLabel::GameInputLabelArrowLeft:
		return BlueSharedString( "ArrowLeft" );
	case GameInputLabel::GameInputLabelArrowUpLeft:
		return BlueSharedString( "ArrowUpLeft" );
	case GameInputLabel::GameInputLabelArrowUpDown:
		return BlueSharedString( "ArrowUpDown" );
	case GameInputLabel::GameInputLabelArrowLeftRight:
		return BlueSharedString( "ArrowLeftRight" );
	case GameInputLabel::GameInputLabelArrowUpDownLeftRight:
		return BlueSharedString( "ArrowUpDownLeftRight" );
	case GameInputLabel::GameInputLabelArrowClockwise:
		return BlueSharedString( "ArrowClockwise" );
	case GameInputLabel::GameInputLabelArrowCounterClockwise:
		return BlueSharedString( "ArrowCounterClockwise" );
	case GameInputLabel::GameInputLabelArrowReturn:
		return BlueSharedString( "ArrowReturn" );
	case GameInputLabel::GameInputLabelIconBranding:
		return BlueSharedString( "IconBranding" );
	case GameInputLabel::GameInputLabelIconHome:
		return BlueSharedString( "IconHome" );
	case GameInputLabel::GameInputLabelIconMenu:
		return BlueSharedString( "IconMenu" );
	case GameInputLabel::GameInputLabelIconCross:
		return BlueSharedString( "IconCross" );
	case GameInputLabel::GameInputLabelIconCircle:
		return BlueSharedString( "IconCircle" );
	case GameInputLabel::GameInputLabelIconSquare:
		return BlueSharedString( "IconSquare" );
	case GameInputLabel::GameInputLabelIconTriangle:
		return BlueSharedString( "IconTriangle" );
	case GameInputLabel::GameInputLabelIconStar:
		return BlueSharedString( "IconStar" );
	case GameInputLabel::GameInputLabelIconDPadUp:
		return BlueSharedString( "IconDPadUp" );
	case GameInputLabel::GameInputLabelIconDPadDown:
		return BlueSharedString( "IconDPadDown" );
	case GameInputLabel::GameInputLabelIconDPadLeft:
		return BlueSharedString( "IconDPadLeft" );
	case GameInputLabel::GameInputLabelIconDPadRight:
		return BlueSharedString( "IconDPadRight" );
	case GameInputLabel::GameInputLabelIconDialClockwise:
		return BlueSharedString( "IconDialClockwise" );
	case GameInputLabel::GameInputLabelIconDialCounterClockwise:
		return BlueSharedString( "IconDialCounterClockwise" );
	case GameInputLabel::GameInputLabelIconSliderLeftRight:
		return BlueSharedString( "IconSliderLeftRight" );
	case GameInputLabel::GameInputLabelIconSliderUpDown:
		return BlueSharedString( "IconSliderUpDown" );
	case GameInputLabel::GameInputLabelIconWheelUpDown:
		return BlueSharedString( "IconWheelUpDown" );
	case GameInputLabel::GameInputLabelIconPlus:
		return BlueSharedString( "IconPlus" );
	case GameInputLabel::GameInputLabelIconMinus:
		return BlueSharedString( "IconMinus" );
	case GameInputLabel::GameInputLabelIconSuspension:
		return BlueSharedString( "IconSuspension" );
	case GameInputLabel::GameInputLabelHome:
		return BlueSharedString( "Home" );
	case GameInputLabel::GameInputLabelGuide:
		return BlueSharedString( "Guide" );
	case GameInputLabel::GameInputLabelMode:
		return BlueSharedString( "Mode" );
	case GameInputLabel::GameInputLabelSelect:
		return BlueSharedString( "Select" );
	case GameInputLabel::GameInputLabelMenu:
		return BlueSharedString( "Menu" );
	case GameInputLabel::GameInputLabelView:
		return BlueSharedString( "View" );
	case GameInputLabel::GameInputLabelBack:
		return BlueSharedString( "Back" );
	case GameInputLabel::GameInputLabelStart:
		return BlueSharedString( "Start" );
	case GameInputLabel::GameInputLabelOptions:
		return BlueSharedString( "Options" );
	case GameInputLabel::GameInputLabelShare:
		return BlueSharedString( "Share" );
	case GameInputLabel::GameInputLabelUp:
		return BlueSharedString( "Up" );
	case GameInputLabel::GameInputLabelDown:
		return BlueSharedString( "Down" );
	case GameInputLabel::GameInputLabelLeft:
		return BlueSharedString( "Left" );
	case GameInputLabel::GameInputLabelRight:
		return BlueSharedString( "Right" );
	case GameInputLabel::GameInputLabelLB:
		return BlueSharedString( "LB" );
	case GameInputLabel::GameInputLabelLT:
		return BlueSharedString( "LT" );
	case GameInputLabel::GameInputLabelLSB:
		return BlueSharedString( "LSB" );
	case GameInputLabel::GameInputLabelL1:
		return BlueSharedString( "L1" );
	case GameInputLabel::GameInputLabelL2:
		return BlueSharedString( "L2" );
	case GameInputLabel::GameInputLabelL3:
		return BlueSharedString( "L3" );
	case GameInputLabel::GameInputLabelRB:
		return BlueSharedString( "RB" );
	case GameInputLabel::GameInputLabelRT:
		return BlueSharedString( "RT" );
	case GameInputLabel::GameInputLabelRSB:
		return BlueSharedString( "RSB" );
	case GameInputLabel::GameInputLabelR1:
		return BlueSharedString( "R1" );
	case GameInputLabel::GameInputLabelR2:
		return BlueSharedString( "R2" );
	case GameInputLabel::GameInputLabelR3:
		return BlueSharedString( "R3" );
	case GameInputLabel::GameInputLabelPaddleLeft1:
		return BlueSharedString( "PaddleLeft1" );
	case GameInputLabel::GameInputLabelPaddleLeft2:
		return BlueSharedString( "PaddleLeft2" );
	case GameInputLabel::GameInputLabelPaddleRight1:
		return BlueSharedString( "PaddleRight1" );
	case GameInputLabel::GameInputLabelPaddleRight2:
		return BlueSharedString( "PaddleRight2" );
	default:
		return BlueSharedString( "Unknown" );
	}
}

BlueSharedString GetCustomName( std::string typeDescriptor, uint32_t index )
{
	std::string ss = "Custom " + typeDescriptor + " " + std::to_string( index );
	return BlueSharedString( ss );
}

void GetButtonIdentifiers( const GameInputControllerInfo* info, std::vector<BlueSharedString>& buttonNames )
{
	buttonNames.clear();

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
	buttonNames.reserve( buttonCount );
	uint32_t unknownButtonIndex = 0;

	for( uint32_t i = 0; i < buttonCount; ++i )
	{
		if( labels[i] == GameInputLabel::GameInputLabelUnknown || labels[i] == GameInputLabel::GameInputLabelNone )
		{
			buttonNames.push_back( GetCustomName( "Button", unknownButtonIndex++ ) );
			continue;
		}
		buttonNames.push_back( BlueSharedString( GetName( labels[i] ) ) );
	}
}

void GetAxisIdentifiers( const GameInputControllerInfo* info, std::vector<BlueSharedString>& axisNames )
{
	axisNames.clear();
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
	axisNames.reserve( axisCount );
	uint32_t unknownAxisIndex = 0;
	for( uint32_t i = 0; i < axisCount; ++i )
	{
		if( labels[i] == GameInputLabel::GameInputLabelUnknown || labels[i] == GameInputLabel::GameInputLabelNone )
		{
			axisNames.push_back( GetCustomName( "Axis", unknownAxisIndex++ ) );
			continue;
		}
		axisNames.push_back( BlueSharedString( GetName( labels[i] ) ) );
	}
}

void GetSwitchIdentifiers( const GameInputControllerInfo* info, std::vector<BlueSharedString>& switchNames )
{
	switchNames.clear();
	if( !info )
	{
		return;
	}

	// GameInput has no per-switch name: GameInputControllerSwitchInfo only carries a label
	// per switch *position*. Synthesize one name per switch element instead.
	const uint32_t switchCount = info->controllerSwitchCount;
	switchNames.reserve( switchCount );
	for( uint32_t i = 0; i < switchCount; ++i )
	{
		switchNames.push_back( GetCustomName( "Dpad", i ) );
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
// Converts the 32 byte device ID from GameInput into a string to be used as a unique identifier for devices.
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
			CCP_LOGNOTICE( "InputHandlerWin: Device '%s' reconnected", identifier.name.c_str() );
		}
		else
		{
			DeviceSlot slot = {
				device,
				false,
				identifier
			};

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
	auto state = self->ReadDeviceState( reading );
	{
		auto foundSlot = self->GetDeviceSlot( device );
		if( foundSlot )
		{
			std::unique_lock<std::shared_mutex> lock( self->m_readingMutex );
			foundSlot->accumulatedStates.push_back( std::move( state ) );
		}
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
	if( info->controllerInfo != nullptr )
	{
		// extract the button/axis/switch names from the controller info if available
		Mapping::GetButtonIdentifiers( info->controllerInfo, identifier.buttons );
		Mapping::GetAxisIdentifiers( info->controllerInfo, identifier.axes );
		Mapping::GetSwitchIdentifiers( info->controllerInfo, identifier.switches );
	}
	return identifier;
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
// ReadDeviceState  –  get the most recent reading for a device
// ---------------------------------------------------------------------------
Events::State InputHandlerWin::ReadDeviceState( IGameInputReading* reading )
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

	if( buttonCount > 0 )
	{
		auto buttonReading = std::make_unique<bool[]>( buttonCount );
		reading->GetControllerButtonState( buttonCount, buttonReading.get() );

		state.buttons.resize( buttonCount );
		for( uint32_t index = 0; index < buttonCount; ++index )
		{
			state.buttons[index].pressed = buttonReading[index] != 0;
		}
	}

	if( axisCount > 0 )
	{
		auto axisReading = std::make_unique<float[]>( axisCount );
		reading->GetControllerAxisState( axisCount, axisReading.get() );
		state.axis.resize( axisCount );
		for( uint32_t index = 0; index < axisCount; ++index )
		{
			state.axis[index].value = axisReading[index];
		}
	}

	if( switchCount > 0 )
	{
		auto switchReading = std::make_unique<GameInputSwitchPosition[]>( switchCount );
		state.switches.resize( switchCount );
		reading->GetControllerSwitchState( switchCount, switchReading.get() );
		for( uint32_t index = 0; index < state.switches.size(); ++index )
		{
			state.switches[index].position = Mapping::MapSwitchPosition( switchReading[index] );
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
