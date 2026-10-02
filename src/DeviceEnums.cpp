// Copyright © 2026 CCP ehf.

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

	default:
		// Unknown elements are told apart by the index published alongside them,
		// so callers append that; returning a pointer into a temporary here would dangle.
		return "Unknown";
	}
}

const char* ToGlyphKeyString( InputElementDescriptor element, DeviceFamily family )
{
	const bool playstation = ( family == DeviceFamily::PlayStation );
	const bool nintendo = ( family == DeviceFamily::Nintendo );
	auto getSpecificButtonName = [family]( const char* playstationName, const char* nintendoName, const char* xboxName, const char* other ) -> const char* {
		switch( family )
		{
		case DeviceFamily::Xbox:
			return xboxName;
		case DeviceFamily::PlayStation:
			return playstationName;
		case DeviceFamily::Nintendo:
			return nintendoName;
		default:
			return other;
		}
	};

	switch( element )
	{
	// Nintendo swaps both face pairs relative to the Xbox letter layout.
	case InputElementDescriptor::FaceSouth:
		return getSpecificButtonName( "FaceButtonCross", "FaceButtonB", "FaceButtonA", "FaceButtonA" );
	case InputElementDescriptor::FaceEast:
		return getSpecificButtonName( "FaceButtonCircle", "FaceButtonA", "FaceButtonB", "FaceButtonB" );
	case InputElementDescriptor::FaceWest:
		return getSpecificButtonName( "FaceButtonSquare", "FaceButtonY", "FaceButtonX", "FaceButtonX" );
	case InputElementDescriptor::FaceNorth:
		return getSpecificButtonName( "FaceButtonTriangle", "FaceButtonX", "FaceButtonY", "FaceButtonY" );

	// Switch pads print L/ZL/R/ZR on the shoulders and leave the stick buttons unmarked.
	case InputElementDescriptor::LeftShoulder:
		return getSpecificButtonName( "L1", "L", "LB", "LB" );
	case InputElementDescriptor::LeftTriggerButton:
		return getSpecificButtonName( "L2", "ZL", "LT", "LT" );
	case InputElementDescriptor::LeftStickButton:
		return getSpecificButtonName( "L3", "LSB", "LSB", "LSB" );
	case InputElementDescriptor::RightShoulder:
		return getSpecificButtonName( "R1", "R", "RB", "RB" );
	case InputElementDescriptor::RightTriggerButton:
		return getSpecificButtonName( "R2", "ZR", "RT", "RT" );
	case InputElementDescriptor::RightStickButton:
		return getSpecificButtonName( "R3", "RSB", "RSB", "RSB" );

	case InputElementDescriptor::Start:
		return getSpecificButtonName( "Options", "IconPlus", "Menu", "Start" );
	case InputElementDescriptor::Select:
		return getSpecificButtonName( "Share", "IconMinus", "View", "Back" );
	case InputElementDescriptor::Guide:
		return getSpecificButtonName( "Home", "Home", "Guide", "Mode" );

	// Everything else reads identically on every family.
	default:
		return ToKeyString( element );
	}
}

}
