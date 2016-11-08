#include "Session.hpp"
#include "Configuration.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <boost/python.hpp>

using namespace boost::python;

BOOST_PYTHON_MODULE(Clara)
{
	class_<Session, boost::noncopyable>("Session")
		.def("loadCompilationDatabase", &Session::loadCompilationDatabase)
		.def("hasCompilationDatabase", &Session::hasCompilationDatabase)
		.def("setSourcePaths", &Session::setSourcePaths)
		.def("reloadTool", &Session::reloadTool)
		.def("codeComplete", &Session::codeComplete)
	;
}