#include "DeviceEnums.h"

#include <string>

namespace DeviceEnums
{

const char* ToKeyString( InputElementDescriptor element )
{
	switch( element )
	{
	case InputElementDescriptor::FaceSouth:
		return "FaceSouth";
	case InputElementDescriptor::FaceEast:
		return "FaceEast";
	case InputElementDescriptor::FaceWest:
		return "FaceWest";
	case InputElementDescriptor::FaceNorth:
		return "FaceNorth";

	case InputElementDescriptor::LeftShoulder:
		return "LeftShoulder";
	case InputElementDescriptor::LeftTriggerButton:
		return "LeftTrigger";
	case InputElementDescriptor::LeftStickButton:
		return "LeftStickButton";
	case InputElementDescriptor::RightShoulder:
		return "RightShoulder";
	case InputElementDescriptor::RightTriggerButton:
		return "RightTrigger";
	case InputElementDescriptor::RightStickButton:
		return "RightStickButton";

	case InputElementDescriptor::Start:
		return "Start";
	case InputElementDescriptor::Select:
		return "Select";
	case InputElementDescriptor::Guide:
		return "Guide";

	case InputElementDescriptor::DPadUp:
		return "DPadUp";
	case InputElementDescriptor::DPadDown:
		return "DPadDown";
	case InputElementDescriptor::DPadLeft:
		return "DPadLeft";
	case InputElementDescriptor::DPadRight:
		return "DPadRight";
	case InputElementDescriptor::DPad:
		return "DPad";

	case InputElementDescriptor::PaddleLeft1:
		return "PaddleLeft1";
	case InputElementDescriptor::PaddleLeft2:
		return "PaddleLeft2";
	case InputElementDescriptor::PaddleRight1:
		return "PaddleRight1";
	case InputElementDescriptor::PaddleRight2:
		return "PaddleRight2";

	case InputElementDescriptor::LeftStickX:
		return "LeftStickX";
	case InputElementDescriptor::LeftStickY:
		return "LeftStickY";
	case InputElementDescriptor::RightStickX:
		return "RightStickX";
	case InputElementDescriptor::RightStickY:
		return "RightStickY";
	case InputElementDescriptor::LeftTriggerAxis:
		return "LeftTriggerAxis";
	case InputElementDescriptor::RightTriggerAxis:
		return "RightTriggerAxis";

	case InputElementDescriptor::LetterA:
		return "LetterA";
	case InputElementDescriptor::LetterB:
		return "LetterB";
	case InputElementDescriptor::LetterC:
		return "LetterC";
	case InputElementDescriptor::LetterD:
		return "LetterD";
	case InputElementDescriptor::LetterE:
		return "LetterE";
	case InputElementDescriptor::LetterF:
		return "LetterF";
	case InputElementDescriptor::LetterG:
		return "LetterG";
	case InputElementDescriptor::LetterH:
		return "LetterH";
	case InputElementDescriptor::LetterI:
		return "LetterI";
	case InputElementDescriptor::LetterJ:
		return "LetterJ";
	case InputElementDescriptor::LetterK:
		return "LetterK";
	case InputElementDescriptor::LetterL:
		return "LetterL";
	case InputElementDescriptor::LetterM:
		return "LetterM";
	case InputElementDescriptor::LetterN:
		return "LetterN";
	case InputElementDescriptor::LetterO:
		return "LetterO";
	case InputElementDescriptor::LetterP:
		return "LetterP";
	case InputElementDescriptor::LetterQ:
		return "LetterQ";
	case InputElementDescriptor::LetterR:
		return "LetterR";
	case InputElementDescriptor::LetterS:
		return "LetterS";
	case InputElementDescriptor::LetterT:
		return "LetterT";
	case InputElementDescriptor::LetterU:
		return "LetterU";
	case InputElementDescriptor::LetterV:
		return "LetterV";
	case InputElementDescriptor::LetterW:
		return "LetterW";
	case InputElementDescriptor::LetterX:
		return "LetterX";
	case InputElementDescriptor::LetterY:
		return "LetterY";
	case InputElementDescriptor::LetterZ:
		return "LetterZ";

	case InputElementDescriptor::Number0:
		return "Number0";
	case InputElementDescriptor::Number1:
		return "Number1";
	case InputElementDescriptor::Number2:
		return "Number2";
	case InputElementDescriptor::Number3:
		return "Number3";
	case InputElementDescriptor::Number4:
		return "Number4";
	case InputElementDescriptor::Number5:
		return "Number5";
	case InputElementDescriptor::Number6:
		return "Number6";
	case InputElementDescriptor::Number7:
		return "Number7";
	case InputElementDescriptor::Number8:
		return "Number8";
	case InputElementDescriptor::Number9:
		return "Number9";

	case InputElementDescriptor::ArrowUp:
		return "ArrowUp";
	case InputElementDescriptor::ArrowUpRight:
		return "ArrowUpRight";
	case InputElementDescriptor::ArrowRight:
		return "ArrowRight";
	case InputElementDescriptor::ArrowDownRight:
		return "ArrowDownRight";
	case InputElementDescriptor::ArrowDown:
		return "ArrowDown";
	case InputElementDescriptor::ArrowDownLeft:
		return "ArrowDownLeft";
	case InputElementDescriptor::ArrowLeft:
		return "ArrowLeft";
	case InputElementDescriptor::ArrowUpLeft:
		return "ArrowUpLeft";
	case InputElementDescriptor::ArrowUpDown:
		return "ArrowUpDown";
	case InputElementDescriptor::ArrowLeftRight:
		return "ArrowLeftRight";
	case InputElementDescriptor::ArrowUpDownLeftRight:
		return "ArrowUpDownLeftRight";
	case InputElementDescriptor::ArrowClockwise:
		return "ArrowClockwise";
	case InputElementDescriptor::ArrowCounterClockwise:
		return "ArrowCounterClockwise";
	case InputElementDescriptor::ArrowReturn:
		return "ArrowReturn";

	case InputElementDescriptor::IconBranding:
		return "IconBranding";
	case InputElementDescriptor::IconStar:
		return "IconStar";
	case InputElementDescriptor::IconPlus:
		return "IconPlus";
	case InputElementDescriptor::IconMinus:
		return "IconMinus";
	case InputElementDescriptor::IconSuspension:
		return "IconSuspension";
	case InputElementDescriptor::IconDialClockwise:
		return "IconDialClockwise";
	case InputElementDescriptor::IconDialCounterClockwise:
		return "IconDialCounterClockwise";
	case InputElementDescriptor::IconSliderLeftRight:
		return "IconSliderLeftRight";
	case InputElementDescriptor::IconSliderUpDown:
		return "IconSliderUpDown";
	case InputElementDescriptor::IconWheelUpDown:
		return "IconWheelUpDown";

	case InputElementDescriptor::Unknown:
	default:
		return "Unknown";
	}
}

const char* ToGlyphKeyString( InputElementDescriptor element, DeviceFamily family )
{
	const bool playstation = ( family == DeviceFamily::PlayStation );
	const bool nintendo = ( family == DeviceFamily::Nintendo );

	switch( element )
	{
	// Nintendo swaps both face pairs relative to the Xbox letter layout.
	case InputElementDescriptor::FaceSouth:
		if( playstation ) return "FaceButtonCross";
		return nintendo ? "FaceButtonB" : "FaceButtonA";
	case InputElementDescriptor::FaceEast:
		if( playstation ) return "FaceButtonCircle";
		return nintendo ? "FaceButtonA" : "FaceButtonB";
	case InputElementDescriptor::FaceWest:
		if( playstation ) return "FaceButtonSquare";
		return nintendo ? "FaceButtonY" : "FaceButtonX";
	case InputElementDescriptor::FaceNorth:
		if( playstation ) return "FaceButtonTriangle";
		return nintendo ? "FaceButtonX" : "FaceButtonY";

	// Switch pads print L/ZL/R/ZR on the shoulders and leave the stick buttons unmarked.
	case InputElementDescriptor::LeftShoulder:
		if( playstation ) return "L1";
		return nintendo ? "L" : "LB";
	case InputElementDescriptor::LeftTriggerButton:
		if( playstation ) return "L2";
		return nintendo ? "ZL" : "LT";
	case InputElementDescriptor::LeftStickButton:
		return playstation ? "L3" : "LSB";
	case InputElementDescriptor::RightShoulder:
		if( playstation ) return "R1";
		return nintendo ? "R" : "RB";
	case InputElementDescriptor::RightTriggerButton:
		if( playstation ) return "R2";
		return nintendo ? "ZR" : "RT";
	case InputElementDescriptor::RightStickButton:
		return playstation ? "R3" : "RSB";

	case InputElementDescriptor::Start:
		switch( family )
		{
		case DeviceFamily::PlayStation:
			return "Options";
		case DeviceFamily::Xbox:
			return "Menu";
		case DeviceFamily::Nintendo:
			return "IconPlus";
		default:
			return "Start";
		}
	case InputElementDescriptor::Select:
		switch( family )
		{
		case DeviceFamily::PlayStation:
			return "Share";
		case DeviceFamily::Xbox:
			return "View";
		case DeviceFamily::Nintendo:
			return "IconMinus";
		default:
			return "Back";
		}
	case InputElementDescriptor::Guide:
		switch( family )
		{
		case DeviceFamily::Xbox:
			return "Guide";
		case DeviceFamily::PlayStation:
		case DeviceFamily::Nintendo:
			return "Home";
		default:
			return "Mode";
		}

	// Everything else reads identically on every family.
	default:
		return ToKeyString( element );
	}
}

}
