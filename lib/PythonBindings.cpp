#include "Session.hpp"
#include "DiagnosticConsumer.hpp"
#include "Configuration.hpp"
#include "SessionOptions.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PYBIND11_PLUGIN(Clara)
{
	using namespace pybind11;
	using namespace Clara;

	module m("Clara", "Clara plugin");

	class_<DiagnosticConsumer>(m, "DiagnosticConsumer")
		.def(init<>())
		.def("beginSourceFile",  &DiagnosticConsumer::beginSourceFile)
		.def("endSourceFile",    &DiagnosticConsumer::EndSourceFile)
		.def("finish",           &DiagnosticConsumer::finish)
		.def("handleDiagnostic", &DiagnosticConsumer::handleDiagnostic)
	;

	class_<SessionOptions>(m, "SessionOptions")
		.def(init<>())
		.def_readwrite("logCallback",                      &SessionOptions::logCallback, "A callable python object that accepts strings as single argument.")
		.def_readwrite("codeCompleteCallback",             &SessionOptions::codeCompleteCallback, "A callable python object that accepts a list of pairs of strings.")
		.def_readwrite("filename",                         &SessionOptions::filename, "The filename of the session.")
		.def_readwrite("systemHeaders",                    &SessionOptions::systemHeaders, "The system headers for the session.")
		.def_readwrite("builtinHeaders",                   &SessionOptions::builtinHeaders, "The builtin headers for the session.")
		.def_readwrite("jsonCompileCommands",              &SessionOptions::jsonCompileCommands, "The directory where the compile commands file resides (in JSON)")
		.def_readwrite("languageStandard",                 &SessionOptions::languageStandard, "The language standard (lang_cxx11, lang_cxx14, lang_cxx1z)")
		.def_readwrite("codeCompleteIncludeMacros",        &SessionOptions::codeCompleteIncludeMacros)
		.def_readwrite("codeCompleteIncludeCodePatterns",  &SessionOptions::codeCompleteIncludeCodePatterns)
		.def_readwrite("codeCompleteIncludeGlobals",       &SessionOptions::codeCompleteIncludeGlobals)
		.def_readwrite("codeCompleteIncludeBriefComments", &SessionOptions::codeCompleteIncludeBriefComments)
	;

	class_<Session>(m, "Session")
		.def(init<const SessionOptions&>())
		// .def_readwrite("reporter",    &Session::reporter)
		.def("codeComplete",          &Session::codeComplete)
		.def("codeCompleteAsync",     &Session::codeCompleteAsync)
		// .def("cancelAsyncCompletion", &Session::cancelAsyncCompletion)
		.def("filename",              &Session::getFilename/*, return_value_policy<copy_const_reference>()*/)
	;

	return m.ptr();
}