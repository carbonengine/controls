#include "DeviceEnums.h"

#include <string>

namespace DeviceEnums
{

const char* ToKeyString( InputElement element )
{
	switch( element )
	{
	case InputElement::FaceButtonCross:
		return "FaceButtonCross";
	case InputElement::FaceButtonCircle:
		return "FaceButtonCircle";
	case InputElement::FaceButtonSquare:
		return "FaceButtonSquare";
	case InputElement::FaceButtonTriangle:
		return "FaceButtonTriangle";
	case InputElement::FaceButtonA:
		return "FaceButtonA";
	case InputElement::FaceButtonB:
		return "FaceButtonB";
	case InputElement::FaceButtonX:
		return "FaceButtonX";
	case InputElement::FaceButtonY:
		return "FaceButtonY";

	case InputElement::L1:
		return "L1";
	case InputElement::L2:
		return "L2";
	case InputElement::L3:
		return "L3";
	case InputElement::R1:
		return "R1";
	case InputElement::R2:
		return "R2";
	case InputElement::R3:
		return "R3";
	case InputElement::LB:
		return "LB";
	case InputElement::LT:
		return "LT";
	case InputElement::LSB:
		return "LSB";
	case InputElement::RB:
		return "RB";
	case InputElement::RT:
		return "RT";
	case InputElement::RSB:
		return "RSB";

	case InputElement::Start:
		return "Start";
	case InputElement::Back:
		return "Back";
	case InputElement::Select:
		return "Select";
	case InputElement::Menu:
		return "Menu";
	case InputElement::View:
		return "View";
	case InputElement::Options:
		return "Options";
	case InputElement::Share:
		return "Share";
	case InputElement::Guide:
		return "Guide";
	case InputElement::Home:
		return "Home";
	case InputElement::Mode:
		return "Mode";

	case InputElement::DPadUp:
		return "DPadUp";
	case InputElement::DPadDown:
		return "DPadDown";
	case InputElement::DPadLeft:
		return "DPadLeft";
	case InputElement::DPadRight:
		return "DPadRight";
	case InputElement::DPad:
		return "DPad";

	case InputElement::PaddleLeft1:
		return "PaddleLeft1";
	case InputElement::PaddleLeft2:
		return "PaddleLeft2";
	case InputElement::PaddleRight1:
		return "PaddleRight1";
	case InputElement::PaddleRight2:
		return "PaddleRight2";

	case InputElement::LeftStickX:
		return "LeftStickX";
	case InputElement::LeftStickY:
		return "LeftStickY";
	case InputElement::RightStickX:
		return "RightStickX";
	case InputElement::RightStickY:
		return "RightStickY";
	case InputElement::LeftTriggerAxis:
		return "LeftTriggerAxis";
	case InputElement::RightTriggerAxis:
		return "RightTriggerAxis";

	case InputElement::LetterA:
		return "LetterA";
	case InputElement::LetterB:
		return "LetterB";
	case InputElement::LetterC:
		return "LetterC";
	case InputElement::LetterD:
		return "LetterD";
	case InputElement::LetterE:
		return "LetterE";
	case InputElement::LetterF:
		return "LetterF";
	case InputElement::LetterG:
		return "LetterG";
	case InputElement::LetterH:
		return "LetterH";
	case InputElement::LetterI:
		return "LetterI";
	case InputElement::LetterJ:
		return "LetterJ";
	case InputElement::LetterK:
		return "LetterK";
	case InputElement::LetterL:
		return "LetterL";
	case InputElement::LetterM:
		return "LetterM";
	case InputElement::LetterN:
		return "LetterN";
	case InputElement::LetterO:
		return "LetterO";
	case InputElement::LetterP:
		return "LetterP";
	case InputElement::LetterQ:
		return "LetterQ";
	case InputElement::LetterR:
		return "LetterR";
	case InputElement::LetterS:
		return "LetterS";
	case InputElement::LetterT:
		return "LetterT";
	case InputElement::LetterU:
		return "LetterU";
	case InputElement::LetterV:
		return "LetterV";
	case InputElement::LetterW:
		return "LetterW";
	case InputElement::LetterX:
		return "LetterX";
	case InputElement::LetterY:
		return "LetterY";
	case InputElement::LetterZ:
		return "LetterZ";

	case InputElement::Number0:
		return "Number0";
	case InputElement::Number1:
		return "Number1";
	case InputElement::Number2:
		return "Number2";
	case InputElement::Number3:
		return "Number3";
	case InputElement::Number4:
		return "Number4";
	case InputElement::Number5:
		return "Number5";
	case InputElement::Number6:
		return "Number6";
	case InputElement::Number7:
		return "Number7";
	case InputElement::Number8:
		return "Number8";
	case InputElement::Number9:
		return "Number9";

	case InputElement::ArrowUp:
		return "ArrowUp";
	case InputElement::ArrowUpRight:
		return "ArrowUpRight";
	case InputElement::ArrowRight:
		return "ArrowRight";
	case InputElement::ArrowDownRight:
		return "ArrowDownRight";
	case InputElement::ArrowDown:
		return "ArrowDown";
	case InputElement::ArrowDownLeft:
		return "ArrowDownLeft";
	case InputElement::ArrowLeft:
		return "ArrowLeft";
	case InputElement::ArrowUpLeft:
		return "ArrowUpLeft";
	case InputElement::ArrowUpDown:
		return "ArrowUpDown";
	case InputElement::ArrowLeftRight:
		return "ArrowLeftRight";
	case InputElement::ArrowUpDownLeftRight:
		return "ArrowUpDownLeftRight";
	case InputElement::ArrowClockwise:
		return "ArrowClockwise";
	case InputElement::ArrowCounterClockwise:
		return "ArrowCounterClockwise";
	case InputElement::ArrowReturn:
		return "ArrowReturn";

	case InputElement::IconBranding:
		return "IconBranding";
	case InputElement::IconStar:
		return "IconStar";
	case InputElement::IconPlus:
		return "IconPlus";
	case InputElement::IconMinus:
		return "IconMinus";
	case InputElement::IconSuspension:
		return "IconSuspension";
	case InputElement::IconDialClockwise:
		return "IconDialClockwise";
	case InputElement::IconDialCounterClockwise:
		return "IconDialCounterClockwise";
	case InputElement::IconSliderLeftRight:
		return "IconSliderLeftRight";
	case InputElement::IconSliderUpDown:
		return "IconSliderUpDown";
	case InputElement::IconWheelUpDown:
		return "IconWheelUpDown";

	case InputElement::Unknown:
	default:
		return "Unknown";
	}
}

namespace
{

InputElement ResolveFaceButton( ElementPosition position, DeviceFamily family )
{
	const bool playstation = ( family == DeviceFamily::PlayStation );

	switch( position )
	{
	case ElementPosition::FaceSouth:
		return playstation ? InputElement::FaceButtonCross : InputElement::FaceButtonA;
	case ElementPosition::FaceEast:
		return playstation ? InputElement::FaceButtonCircle : InputElement::FaceButtonB;
	case ElementPosition::FaceWest:
		return playstation ? InputElement::FaceButtonSquare : InputElement::FaceButtonX;
	case ElementPosition::FaceNorth:
		return playstation ? InputElement::FaceButtonTriangle : InputElement::FaceButtonY;
	default:
		return InputElement::Unknown;
	}
}

InputElement ResolveShoulder( ElementPosition position, DeviceFamily family )
{
	const bool playstation = ( family == DeviceFamily::PlayStation );

	switch( position )
	{
	case ElementPosition::LeftShoulder:
		return playstation ? InputElement::L1 : InputElement::LB;
	case ElementPosition::LeftTrigger:
		return playstation ? InputElement::L2 : InputElement::LT;
	case ElementPosition::LeftStickButton:
		return playstation ? InputElement::L3 : InputElement::LSB;
	case ElementPosition::RightShoulder:
		return playstation ? InputElement::R1 : InputElement::RB;
	case ElementPosition::RightTrigger:
		return playstation ? InputElement::R2 : InputElement::RT;
	case ElementPosition::RightStickButton:
		return playstation ? InputElement::R3 : InputElement::RSB;
	default:
		return InputElement::Unknown;
	}
}

InputElement ResolveSystemButton( ElementPosition position, DeviceFamily family )
{
	switch( position )
	{
	case ElementPosition::Start:
		switch( family )
		{
		case DeviceFamily::PlayStation:
			return InputElement::Options;
		case DeviceFamily::Xbox:
			return InputElement::Menu;
		case DeviceFamily::Nintendo:
			return InputElement::Start;
		default:
			return InputElement::Start;
		}
	case ElementPosition::Select:
		switch( family )
		{
		case DeviceFamily::PlayStation:
			return InputElement::Share;
		case DeviceFamily::Xbox:
			return InputElement::View;
		case DeviceFamily::Nintendo:
			return InputElement::Select;
		default:
			return InputElement::Back;
		}
	case ElementPosition::Guide:
		switch( family )
		{
		case DeviceFamily::PlayStation:
			return InputElement::Home;
		case DeviceFamily::Xbox:
			return InputElement::Guide;
		case DeviceFamily::Nintendo:
			return InputElement::Home;
		default:
			return InputElement::Mode;
		}
	default:
		return InputElement::Unknown;
	}
}

}

InputElement ResolveElement( ElementPosition position, DeviceFamily family )
{
	switch( position )
	{
	case ElementPosition::FaceSouth:
	case ElementPosition::FaceEast:
	case ElementPosition::FaceWest:
	case ElementPosition::FaceNorth:
		return ResolveFaceButton( position, family );

	case ElementPosition::LeftShoulder:
	case ElementPosition::LeftTrigger:
	case ElementPosition::LeftStickButton:
	case ElementPosition::RightShoulder:
	case ElementPosition::RightTrigger:
	case ElementPosition::RightStickButton:
		return ResolveShoulder( position, family );

	case ElementPosition::Start:
	case ElementPosition::Select:
	case ElementPosition::Guide:
		return ResolveSystemButton( position, family );

	// The remaining positions carry no glyph variation between families.
	case ElementPosition::DPadUp:
		return InputElement::DPadUp;
	case ElementPosition::DPadDown:
		return InputElement::DPadDown;
	case ElementPosition::DPadLeft:
		return InputElement::DPadLeft;
	case ElementPosition::DPadRight:
		return InputElement::DPadRight;
	case ElementPosition::DPad:
		return InputElement::DPad;

	case ElementPosition::PaddleLeft1:
		return InputElement::PaddleLeft1;
	case ElementPosition::PaddleLeft2:
		return InputElement::PaddleLeft2;
	case ElementPosition::PaddleRight1:
		return InputElement::PaddleRight1;
	case ElementPosition::PaddleRight2:
		return InputElement::PaddleRight2;

	case ElementPosition::LeftStickX:
		return InputElement::LeftStickX;
	case ElementPosition::LeftStickY:
		return InputElement::LeftStickY;
	case ElementPosition::RightStickX:
		return InputElement::RightStickX;
	case ElementPosition::RightStickY:
		return InputElement::RightStickY;
	case ElementPosition::LeftTriggerAxis:
		return InputElement::LeftTriggerAxis;
	case ElementPosition::RightTriggerAxis:
		return InputElement::RightTriggerAxis;

	case ElementPosition::Unknown:
	default:
		return InputElement::Unknown;
	}
}

}
