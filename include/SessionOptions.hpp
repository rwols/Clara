#pragma once

#include <vector>
#include <string>

#include "DiagnosticConsumer.hpp"

namespace Clara {

struct SessionOptions
{
	Clara::DiagnosticConsumer* diagnosticConsumer = nullptr;
	std::string filename;
	std::vector<std::string> systemHeaders;
	std::string builtinHeaders;
	std::string jsonCompileCommands;
	bool cxx11 = true;
	bool cxx14 = true;
	bool cxx1z = false;
	bool codeCompleteIncludeMacros = true;
	bool codeCompleteIncludeCodePatterns = true;
	bool codeCompleteIncludeGlobals = true;
	bool codeCompleteIncludeBriefComments = true;
};

} // namespace Clara