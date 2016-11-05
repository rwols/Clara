#include "Project.hpp"
#include "Configuration.hpp"
#include <boost/python.hpp>

using namespace boost::python;

BOOST_PYTHON_MODULE(Clara)
{
	class_<Project>("Project", init<std::string>())
		.def("greet", &Project::greet)
		.def("set", &Project::set)
	;
}