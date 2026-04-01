#include "StdAfx.h"
#include "ControlManager.h"

const char* g_moduleName = "_carbon_controls";

static void StartDLL()
{
	CCP_LOG( "%s starting", CCP_STRINGIZE( CCP_CONCATENATE( _carbon_controls, CCP_BUILD_FLAVOR ) ) );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
}

#if BLUE_WITH_PYTHON

PyMODINIT_FUNC
	CCP_CONCATENATE( CCP_CONCATENATE( PyInit_, _carbon_controls ), CCP_BUILD_FLAVOR )()
{
	StartDLL();
	static PyMethodDef dummyMethods[] = { 0 };

	// put myself into python as a module
	static struct PyModuleDef carbonControlsDef = {
		PyModuleDef_HEAD_INIT,
		CCP_STRINGIZE( CCP_CONCATENATE( _carbon_controls, CCP_BUILD_FLAVOR ) ),
		"",
		-1,
		dummyMethods
	};
	PyObject* module = PyModule_Create( &carbonControlsDef );
	if( module )
	{
		BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs(), BlueRegistration::GetEnumRegs(), BlueRegistration::GetTestRegs(), BlueRegistration::GetThunkerRegs(), BlueRegistration::GetFuncSignatures() );
		BlueRegisterObjectsToModule( module, BlueRegistration::GetObjectRegs() );
	}
	return module;
}

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
