#include "Session.hpp"
#include "DiagnosticConsumer.hpp"
#include "Configuration.hpp"
#include "CompletionResultListToPythonList.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <boost/python.hpp>

BOOST_PYTHON_MODULE(cpp)
{
	using namespace boost::python;

	PyEval_InitThreads();

	//class_<std::vector<std::pair<std::string, std::string>>>("CompletionResultList")
	//	.def(vector_indexing_suite<std::vector<std::pair<std::string, std::string>>>())
	//;

	class_<Clara::DiagnosticConsumer>("DiagnosticConsumer")
		.def("beginSourceFile", &Clara::DiagnosticConsumer::beginSourceFile)
		.def("endSourceFile", &Clara::DiagnosticConsumer::EndSourceFile)
		.def("finish", &Clara::DiagnosticConsumer::finish)
		.def("handleDiagnostic", &Clara::DiagnosticConsumer::handleDiagnostic)
	;

	class_<Clara::Session, boost::noncopyable>("Session", init<Clara::DiagnosticConsumer&, const std::string&>())
		.def(init<Clara::DiagnosticConsumer&, const std::string&, const std::string&>())
		.def(init<const std::string&>())
		.def(init<const std::string&, const std::string&>())
		.def_readwrite("reporter", &Clara::Session::reporter)
		.def("codeComplete", &Clara::Session::codeComplete)
		.def("codeCompleteAsync", &Clara::Session::codeCompleteAsync)
		.def("filename", &Clara::Session::getFilename, return_value_policy<copy_const_reference>())mAction.EndSourceFile()
	;

	to_python_converter<std::vector<std::pair<std::string, std::string>>, Clara::CompletionResultListToPythonList>();
}