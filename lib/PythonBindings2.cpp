#include <boost/python.hpp>

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