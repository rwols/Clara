#pragma once

#include <Python.h>

namespace Clara {

class PythonGILEnsurer 
{
public:	
	PythonGILEnsurer();
	~PythonGILEnsurer();
private:
	PyGILState_STATE mState;
};

} // namespace Clara

