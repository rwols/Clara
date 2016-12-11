#pragma once

#include <vector>
#include <string>
#include <iosfwd>
#include <clang/Frontend/LangStandard.h>
#include "PyBind11.hpp"

namespace Clara {

class DiagnosticConsumer;

struct SessionOptions
{
	pybind11::object logCallback;
	pybind11::object codeCompleteCallback;
	pybind11::object diagnosticCallback;
	std::string filename;
	std::vector<std::string> systemHeaders;
	std::vector<std::string> frameworks;
	std::string builtinHeaders;
	std::string jsonCompileCommands;
	std::string workingDirectory;
	std::vector<std::string> invocation;
	clang::LangStandard::Kind languageStandard;
	bool codeCompleteIncludeMacros = true;
	bool codeCompleteIncludeCodePatterns = true;
	bool codeCompleteIncludeGlobals = true;
	bool codeCompleteIncludeBriefComments = true;
};

/**
 * @brief      Debug output for a SessionOptions object.
 *
 * @param      os       The output stream.
 * @param[in]  options  The object.
 *
 * @return     The modified output stream.
 */
std::ostream& operator << (std::ostream& os, const SessionOptions& options);

} // namespace Clara