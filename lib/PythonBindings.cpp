#include "Session.hpp"
#include "DiagnosticConsumer.hpp"
#include "Configuration.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <boost/python.hpp>

using namespace boost::python;

BOOST_PYTHON_MODULE(Clara)
{
	class_<Clara::DiagnosticConsumer>("DiagnosticConsumer")
		.def("beginSourceFile", &Clara::DiagnosticConsumer::beginSourceFile)
		.def("endSourceFile", &Clara::DiagnosticConsumer::EndSourceFile)
		.def("finish", &Clara::DiagnosticConsumer::finish)
		.def("handleDiagnostic", &Clara::DiagnosticConsumer::handleDiagnostic)
	;

	class_<Clara::Session, boost::noncopyable>("Session")
		.def("loadCompilationDatabase", &Clara::Session::loadCompilationDatabase)
		.def("hasCompilationDatabase", &Clara::Session::hasCompilationDatabase)
		.def("setSourcePaths", &Clara::Session::setSourcePaths)
		.def("reloadTool", &Clara::Session::reloadTool)
		.def("codeComplete", &Clara::Session::codeComplete)
	;
}