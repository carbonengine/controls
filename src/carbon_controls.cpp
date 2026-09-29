#include "StdAfx.h"
#include "ControlManager.h"

BLUE_STANDARD_MODULE_INIT( _carbon_controls )

static void StartDLL()
{
	CCP_LOG( "%s starting", CCP_STRINGIZE( CCP_CONCATENATE( _carbon_controls, CCP_BUILD_FLAVOR ) ) );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
}

#if BLUE_WITH_PYTHON

ControlManagerPtr GetControlManager()
{
	static ControlManagerPtr instance;
	if( !instance )
	{
		instance.CreateInstance();
	}
	return instance;
}

MAP_FUNCTION_AND_WRAP( "GetControlManager", GetControlManager, "Returns the Control Manager" );
#endif
