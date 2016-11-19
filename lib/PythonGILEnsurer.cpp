#include "PythonGILEnsurer.hpp"

using namespace Clara;

PythonGILEnsurer::PythonGILEnsurer() 
: mState(PyGILState_Ensure())
{

}

PythonGILEnsurer::~PythonGILEnsurer() 
{
	PyGILState_Release(mState);
}
