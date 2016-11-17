#include <boost/python.hpp>

struct PythonContext
{
	PythonContext()
	{
		Py_Initialize();
	}
	~PythonContext()
	{
		Py_Finalize();
	}
};

int main()
{
	PythonContext context;
}