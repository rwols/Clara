#pragma once

#include <vector>
#include <string>
#include <boost/python/object.hpp>

#include "DiagnosticConsumer.hpp"

namespace Clara {

struct SessionOptions
{
	boost::python::object logCallback;
	boost::python::object codeCompleteCallback;
	Clara::DiagnosticConsumer* diagnosticConsumer = nullptr;
	std::string filename;
	std::vector<std::string> systemHeaders;
	std::string builtinHeaders;
	std::string jsonCompileCommands;
	clang::LangStandard languageStandard;
	// bool cxx11 = true;
	// bool cxx14 = true;
	// bool cxx1z = false;
	bool codeCompleteIncludeMacros = true;
	bool codeCompleteIncludeCodePatterns = true;
	bool codeCompleteIncludeGlobals = true;
	bool codeCompleteIncludeBriefComments = true;
};

} // namespace Clara