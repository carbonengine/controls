#include "StdAfx.h"
#include "ControlManager.h"

const char* g_moduleName = "_carbon_controls";

static void StartDLL()
{
	CCP_LOG( "_carbon_controls starting" );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
}

#if BLUE_WITH_PYTHON

PyMODINIT_FUNC
	PyInit__carbon_controls()
{
	StartDLL();
	static PyMethodDef dummyMethods[] = { 0 };

	// put myself into python as a module
	static struct PyModuleDef carbonControlsDef = {
		PyModuleDef_HEAD_INIT,
		"_carbon_controls",
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
