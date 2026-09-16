#ifdef __APPLE__
#include "InputMappingApple.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace
{
// A dpad's child axes are analog for thumbsticks/touchpads and digital for a real d-pad;
// only the analog ones are published as axes, the digital ones as a switch instead.
bool IsAnalogDpad( GCControllerDirectionPad* dpad )
{
	return dpad != nil && dpad.xAxis != nil && dpad.yAxis != nil && dpad.xAxis.isAnalog;
}
}

namespace InputMapping
{
DeviceEnums::InputElementDescriptor ElementForKey( NSString* key )
{
	using Element = DeviceEnums::InputElementDescriptor;

	if( key == nil )
	{
		return Element::Unknown;
	}

	if( [key isEqualToString:GCInputButtonA] ) return Element::FaceSouth;
	if( [key isEqualToString:GCInputButtonB] ) return Element::FaceEast;
	if( [key isEqualToString:GCInputButtonX] ) return Element::FaceWest;
	if( [key isEqualToString:GCInputButtonY] ) return Element::FaceNorth;

	if( [key isEqualToString:GCInputLeftShoulder] ) return Element::LeftShoulder;
	if( [key isEqualToString:GCInputRightShoulder] ) return Element::RightShoulder;
	if( [key isEqualToString:GCInputLeftThumbstickButton] ) return Element::LeftStickButton;
	if( [key isEqualToString:GCInputRightThumbstickButton] ) return Element::RightStickButton;

	if( [key isEqualToString:GCInputButtonMenu] ) return Element::Start;
	if( [key isEqualToString:GCInputButtonOptions] ) return Element::Select;
	if( [key isEqualToString:GCInputButtonHome] ) return Element::Guide;

	if( [key isEqualToString:GCInputDirectionPad] ) return Element::DPad;

	// Triggers are analog, so they live in the axis dimension.
	if( [key isEqualToString:GCInputLeftTrigger] ) return Element::LeftTriggerAxis;
	if( [key isEqualToString:GCInputRightTrigger] ) return Element::RightTriggerAxis;

	if( [key isEqualToString:GCInputXboxPaddleOne] ) return Element::PaddleLeft1;
	if( [key isEqualToString:GCInputXboxPaddleTwo] ) return Element::PaddleLeft2;
	if( [key isEqualToString:GCInputXboxPaddleThree] ) return Element::PaddleRight1;
	if( [key isEqualToString:GCInputXboxPaddleFour] ) return Element::PaddleRight2;

	return Element::Unknown;
}

DeviceEnums::InputElementDescriptor ElementForThumbstickAxis( NSString* parentKey, bool isXAxis )
{
	using Element = DeviceEnums::InputElementDescriptor;

	if( parentKey == nil )
	{
		return Element::Unknown;
	}
	if( [parentKey isEqualToString:GCInputLeftThumbstick] )
	{
		return isXAxis ? Element::LeftStickX : Element::LeftStickY;
	}
	if( [parentKey isEqualToString:GCInputRightThumbstick] )
	{
		return isXAxis ? Element::RightStickX : Element::RightStickY;
	}
	return Element::Unknown;
}

DeviceEnums::InputElementDescriptor TriggerButtonElement( DeviceEnums::InputElementDescriptor axisElement )
{
	using Element = DeviceEnums::InputElementDescriptor;
	switch( axisElement )
	{
	case Element::LeftTriggerAxis: return Element::LeftTriggerButton;
	case Element::RightTriggerAxis: return Element::RightTriggerButton;
	default: return Element::Unknown;
	}
}

DeviceEnums::DeviceFamily GetDeviceFamily( GCController* controller )
{
	NSString* category = controller.productCategory;
	if( category == nil )
	{
		return DeviceEnums::DeviceFamily::Generic;
	}

	if( [category containsString:@"DualSense"] ||
		[category containsString:@"DualShock"] ||
		[category containsString:@"PlayStation"] )
	{
		return DeviceEnums::DeviceFamily::PlayStation;
	}
	if( [category containsString:@"Xbox"] )
	{
		return DeviceEnums::DeviceFamily::Xbox;
	}
	if( [category containsString:@"Switch"] ||
		[category containsString:@"Joy-Con"] ||
		[category containsString:@"Nintendo"] )
	{
		return DeviceEnums::DeviceFamily::Nintendo;
	}
	return DeviceEnums::DeviceFamily::Generic;
}

std::string SanitizeForDeviceID( NSString* input )
{
	if( input == nil )
	{
		return std::string();
	}
	std::string result;
	result.reserve( [input length] );
	const char* utf8 = [input UTF8String];
	if( utf8 == nullptr )
	{
		return result;
	}
	for( const char* c = utf8; *c != '\0'; ++c )
	{
		const unsigned char uc = static_cast<unsigned char>( *c );
		if( ( uc >= 'A' && uc <= 'Z' ) || ( uc >= 'a' && uc <= 'z' ) || ( uc >= '0' && uc <= '9' ) )
		{
			result.push_back( static_cast<char>( uc ) );
		}
		else if( uc == '-' || uc == '_' )
		{
			result.push_back( static_cast<char>( uc ) );
		}
	}
	return result;
}
}


namespace ButtonHandling
{
std::vector<ButtonSource> GetButtonSources( GCController* controller )
{
	std::vector<ButtonSource> sources;
	if( controller == nil )
	{
		return sources;
	}

	GCPhysicalInputProfile* profile = controller.physicalInputProfile;
	uint32_t unknownCount = 0;

	// An element can be reachable under several keys at once (see GCControllerElement.aliases),
	// so track which underlying elements have already been published to dedupe by identity. This
	// is scratch state for construction only - the sources themselves keep a key, not the element.
	std::unordered_set<GCControllerButtonInput*> seenButtons;

	// .buttons is an NSDictionary with undefined enumeration order; iterate a sorted key
	// list instead so the published order is deterministic.
	NSArray<NSString*>* keys = [profile.buttons.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in keys )
	{
		GCControllerButtonInput* button = profile.buttons[key];
		// Skip buttons that are part of a collection, like a d-pad group; those are
		// reached through their parent element instead.
		if( button.collection != nil )
		{
			continue;
		}
		if( seenButtons.count( button ) > 0 )
		{
			continue;
		}

		DeviceEnums::InputElementDescriptor descriptor = InputMapping::ElementForKey( key );
		if( button.isAnalog )
		{
			// Analog triggers also report a digital press, matching the Windows handler.
			descriptor = InputMapping::TriggerButtonElement( descriptor );
			if( descriptor == DeviceEnums::InputElementDescriptor::Unknown )
			{
				// Deliberately not marking `button` seen: if it's reachable under another alias
				// key that maps to a known descriptor, that alias should still get a chance below.
				continue;
			}
		}
		seenButtons.insert( button );

		ButtonSource source;
		source.kind = ButtonSource::Kind::ProfileButton;
		source.key = key;
		source.descriptor = descriptor;
		source.elementIndex = Events::AssignElementIndex( descriptor, unknownCount );
		sources.push_back( source );
	}

	// Digital d-pads are also published as four individual buttons (matching the Windows
	// handler), in addition to the combined switch published by SwitchHandling below.
	NSArray<NSString*>* dpadKeys = [profile.dpads.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in dpadKeys )
	{
		GCControllerDirectionPad* dpad = profile.dpads[key];
		if( IsAnalogDpad( dpad ) )
		{
			continue;
		}

		const std::tuple<GCControllerButtonInput*, ButtonSource::DpadDirection, DeviceEnums::InputElementDescriptor> directions[] = {
			{ dpad.up, ButtonSource::DpadDirection::Up, DeviceEnums::InputElementDescriptor::DPadUp },
			{ dpad.down, ButtonSource::DpadDirection::Down, DeviceEnums::InputElementDescriptor::DPadDown },
			{ dpad.left, ButtonSource::DpadDirection::Left, DeviceEnums::InputElementDescriptor::DPadLeft },
			{ dpad.right, ButtonSource::DpadDirection::Right, DeviceEnums::InputElementDescriptor::DPadRight },
		};
		for( const auto& [directionButton, direction, descriptor] : directions )
		{
			if( directionButton == nil )
			{
				continue;
			}
			if( !seenButtons.insert( directionButton ).second )
			{
				continue;
			}

			ButtonSource source;
			source.kind = ButtonSource::Kind::DpadDirection;
			source.key = key;
			source.direction = direction;
			source.descriptor = descriptor;
			source.elementIndex = Events::AssignElementIndex( source.descriptor, unknownCount );
			sources.push_back( source );
		}
	}
	return sources;
}

Events::Button Handle( const ButtonSource& source, GCPhysicalInputProfile* profile )
{
	Events::Button button;
	button.descriptor = source.descriptor;
	button.index = source.elementIndex;
	if( profile == nil || source.key == nil )
	{
		return button;
	}

	GCControllerButtonInput* buttonInput = nil;
	switch( source.kind )
	{
	case ButtonSource::Kind::ProfileButton:
		buttonInput = profile.buttons[source.key];
		break;
	case ButtonSource::Kind::DpadDirection:
	{
		GCControllerDirectionPad* dpad = profile.dpads[source.key];
		if( dpad != nil )
		{
			switch( source.direction )
			{
			case ButtonSource::DpadDirection::Up: buttonInput = dpad.up; break;
			case ButtonSource::DpadDirection::Down: buttonInput = dpad.down; break;
			case ButtonSource::DpadDirection::Left: buttonInput = dpad.left; break;
			case ButtonSource::DpadDirection::Right: buttonInput = dpad.right; break;
			}
		}
		break;
	}
	case ButtonSource::Kind::None:
		break;
	}

	button.pressed = buttonInput != nil && buttonInput.isPressed;
	return button;
}
}

namespace AxisHandling
{
std::vector<AxisSource> GetAxisSources( GCController* controller )
{
	std::vector<AxisSource> sources;
	if( controller == nil )
	{
		return sources;
	}

	GCPhysicalInputProfile* profile = controller.physicalInputProfile;
	uint32_t unknownCount = 0;

	// Scratch state for construction only, to dedupe aliased elements by identity - see the
	// equivalent comment in ButtonHandling::GetButtonSources.
	std::unordered_set<GCControllerAxisInput*> seenAxes;

	NSArray<NSString*>* axisKeys = [profile.axes.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in axisKeys )
	{
		GCControllerAxisInput* axis = profile.axes[key];
		// Skip axes that belong to a collection (e.g. dpad X/Y components) - the dpad pass
		// below decides whether the parent becomes a switch or contributes child axes here.
		if( axis.collection != nil )
		{
			continue;
		}
		if( !seenAxes.insert( axis ).second )
		{
			continue;
		}

		AxisSource source;
		source.kind = AxisSource::Kind::ProfileAxis;
		source.key = key;
		source.descriptor = InputMapping::ElementForKey( key );
		source.index = Events::AssignElementIndex( source.descriptor, unknownCount );
		sources.push_back( source );
	}

	NSArray<NSString*>* dpadKeys = [profile.dpads.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in dpadKeys )
	{
		GCControllerDirectionPad* dpad = profile.dpads[key];
		// Apple models thumbsticks and touchpads as direction pads too; only the analog
		// ones contribute axes here, digital d-pads are published as switches instead.
		if( !IsAnalogDpad( dpad ) )
		{
			continue;
		}

		if( seenAxes.insert( dpad.xAxis ).second )
		{
			AxisSource xSource;
			xSource.kind = AxisSource::Kind::DpadAxis;
			xSource.key = key;
			xSource.isXAxis = true;
			xSource.descriptor = InputMapping::ElementForThumbstickAxis( key, true );
			xSource.index = Events::AssignElementIndex( xSource.descriptor, unknownCount );
			sources.push_back( xSource );
		}
		if( seenAxes.insert( dpad.yAxis ).second )
		{
			AxisSource ySource;
			ySource.kind = AxisSource::Kind::DpadAxis;
			ySource.key = key;
			ySource.isXAxis = false;
			ySource.descriptor = InputMapping::ElementForThumbstickAxis( key, false );
			ySource.index = Events::AssignElementIndex( ySource.descriptor, unknownCount );
			sources.push_back( ySource );
		}
	}

	// Analog trigger buttons are appended last, after the plain and thumbstick axes.
	std::unordered_set<GCControllerButtonInput*> seenTriggerButtons;
	NSArray<NSString*>* buttonKeys = [profile.buttons.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in buttonKeys )
	{
		GCControllerButtonInput* button = profile.buttons[key];
		if( button.collection != nil || !button.isAnalog )
		{
			continue;
		}
		if( !seenTriggerButtons.insert( button ).second )
		{
			continue;
		}

		AxisSource source;
		source.kind = AxisSource::Kind::TriggerButton;
		source.key = key;
		source.descriptor = InputMapping::ElementForKey( key );
		source.index = Events::AssignElementIndex( source.descriptor, unknownCount );
		sources.push_back( source );
	}
	return sources;
}

Events::Axis Handle( const AxisSource& source, GCPhysicalInputProfile* profile )
{
	Events::Axis axis;
	axis.descriptor = source.descriptor;
	axis.index = source.index;
	if( profile == nil || source.key == nil )
	{
		return axis;
	}

	switch( source.kind )
	{
	case AxisSource::Kind::ProfileAxis:
	{
		GCControllerAxisInput* axisInput = profile.axes[source.key];
		axis.value = axisInput != nil ? axisInput.value : 0.0f;
		break;
	}
	case AxisSource::Kind::DpadAxis:
	{
		GCControllerDirectionPad* dpad = profile.dpads[source.key];
		GCControllerAxisInput* axisInput = dpad != nil ? ( source.isXAxis ? dpad.xAxis : dpad.yAxis ) : nil;
		axis.value = axisInput != nil ? axisInput.value : 0.0f;
		break;
	}
	case AxisSource::Kind::TriggerButton:
	{
		GCControllerButtonInput* buttonInput = profile.buttons[source.key];
		axis.value = buttonInput != nil ? buttonInput.value : 0.0f;
		break;
	}
	}
	return axis;
}
}

namespace SwitchHandling
{
namespace
{
Events::SwitchPosition MapDpadPosition( GCControllerDirectionPad* dpad )
{
	if( dpad == nil )
	{
		return Events::SwitchPosition::Center;
	}

	const bool up = dpad.up.isPressed;
	const bool down = dpad.down.isPressed;
	const bool right = dpad.right.isPressed;
	const bool left = dpad.left.isPressed;

	if( up && right ) return Events::SwitchPosition::UpRight;
	if( up && left ) return Events::SwitchPosition::UpLeft;
	if( down && right ) return Events::SwitchPosition::DownRight;
	if( down && left ) return Events::SwitchPosition::DownLeft;
	if( up ) return Events::SwitchPosition::Up;
	if( down ) return Events::SwitchPosition::Down;
	if( right ) return Events::SwitchPosition::Right;
	if( left ) return Events::SwitchPosition::Left;
	return Events::SwitchPosition::Center;
}
}

std::vector<SwitchSource> GetSwitchSources( GCController* controller )
{
	std::vector<SwitchSource> sources;
	if( controller == nil )
	{
		return sources;
	}

	GCPhysicalInputProfile* profile = controller.physicalInputProfile;
	// Scratch state for construction only, to dedupe aliased elements by identity - see the
	// equivalent comment in ButtonHandling::GetButtonSources.
	std::unordered_set<GCControllerDirectionPad*> seenDpads;
	NSArray<NSString*>* dpadKeys = [profile.dpads.allKeys sortedArrayUsingSelector:@selector( compare: )];
	for( NSString* key in dpadKeys )
	{
		GCControllerDirectionPad* dpad = profile.dpads[key];
		// Only genuinely digital d-pads land here; analog thumbsticks/touchpads modeled as
		// direction pads are published as axis pairs instead.
		if( IsAnalogDpad( dpad ) )
		{
			continue;
		}
		if( !seenDpads.insert( dpad ).second )
		{
			continue;
		}

		SwitchSource source;
		source.key = key;
		source.index = static_cast<uint32_t>( sources.size() );
		sources.push_back( source );
	}
	return sources;
}

Events::Switch Handle( const SwitchSource& source, GCPhysicalInputProfile* profile )
{
	Events::Switch sw;
	sw.descriptor = source.descriptor;
	sw.index = source.index;
	GCControllerDirectionPad* dpad = ( profile != nil && source.key != nil ) ? profile.dpads[source.key] : nil;
	sw.position = MapDpadPosition( dpad );
	return sw;
}
}

#endif // __APPLE__

