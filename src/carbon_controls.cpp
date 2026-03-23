#if BLUE_WITH_PYTHON
	#include<Python.h>
#endif

#include "IBlueClasses.h"
#include "BlueRegistration.h"

static void StartDLL()
{
	CCP_LOG( "CarbonControls dll starting" );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
}

#if BLUE_WITH_PYTHON

PyMODINIT_FUNC
	PyInit_CarbonControls()
{
	StartDLL();
	static PyMethodDef dummyMethods[] = { 0 };

	// put myself into python as a module
	static struct PyModuleDef carbonControlsDef = {
		PyModuleDef_HEAD_INIT,
		"CarbonControls",
		"",
		-1,
		dummyMethods
	};
	PyObject* module = PyModule_Create( &carbonControlsDef );
	return module;
}

#endif
