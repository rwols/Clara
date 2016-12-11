#include "Session.hpp"
#include "DiagnosticConsumer.hpp"
#include "Configuration.hpp"
#include "SessionOptions.hpp"
#include "CompilationDatabase.hpp"

// #include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PYBIND11_PLUGIN(Clara)
{
	using namespace pybind11;
	using namespace Clara;

	module m("Clara", "Clara plugin");

	class_<CompilationDatabase>(m, "CompilationDatabase")
		.def(init<const std::string&>())
		.def("get", &CompilationDatabase::operator[])
	;

	class_<SessionOptions>(m, "SessionOptions")
		.def(init<>())
		.def_readwrite("diagnosticCallback",               &SessionOptions::diagnosticCallback, "The diagnostic callable handling diagnostic callbacks.")
		.def_readwrite("logCallback",                      &SessionOptions::logCallback, "A callable python object that accepts strings as single argument.")
		.def_readwrite("codeCompleteCallback",             &SessionOptions::codeCompleteCallback, "A callable python object that accepts a list of pairs of strings.")
		.def_readwrite("filename",                         &SessionOptions::filename, "The filename of the session.")
		.def_readwrite("systemHeaders",                    &SessionOptions::systemHeaders, "The system headers for the session.")
		.def_readwrite("frameworks",                       &SessionOptions::frameworks, "The list of directories where OSX system frameworks reside.")
		.def_readwrite("builtinHeaders",                   &SessionOptions::builtinHeaders, "The builtin headers for the session.")
		.def_readwrite("jsonCompileCommands",              &SessionOptions::jsonCompileCommands, "The directory where the compile commands file resides (in JSON)")
		.def_readwrite("invocation",                       &SessionOptions::invocation, "The command line arguments for this translation unit.")
		.def_readwrite("workingDirectory",                 &SessionOptions::workingDirectory, "The working directory for the command line arguments in the invocation.")
		.def_readwrite("languageStandard",                 &SessionOptions::languageStandard, "The language standard (lang_cxx11, lang_cxx14, lang_cxx1z)")
		.def_readwrite("codeCompleteIncludeMacros",        &SessionOptions::codeCompleteIncludeMacros)
		.def_readwrite("codeCompleteIncludeCodePatterns",  &SessionOptions::codeCompleteIncludeCodePatterns)
		.def_readwrite("codeCompleteIncludeGlobals",       &SessionOptions::codeCompleteIncludeGlobals)
		.def_readwrite("codeCompleteIncludeBriefComments", &SessionOptions::codeCompleteIncludeBriefComments)
	;

	class_<Session>(m, "Session")
		.def(init<const SessionOptions&>())
		.def("codeComplete",      &Session::codeComplete)
		.def("codeCompleteAsync", &Session::codeCompleteAsync)
		.def("reparse",           &Session::reparse)
		.def("filename",          &Session::getFilename)
	;

	return m.ptr();
}