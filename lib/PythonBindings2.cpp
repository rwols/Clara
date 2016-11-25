#include <pybind11/pybind11.h>

struct Hello
{
	std::string greet()
	{
		return "Howdy!";
	}
};

BOOST_PYTHON_MODULE(testmodule)
{
	using namespace boost::python;

	class_<Hello>("Hello")
		.def("greet", &Hello::greet)
	;
}