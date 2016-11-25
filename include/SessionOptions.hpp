#pragma once

#include <vector>
#include <string>
#include <boost/python/object.hpp>
#include <iosfwd>
#include <clang/Frontend/LangStandard.h>

namespace Clara {

class DiagnosticConsumer;

struct SessionOptions
{
	boost::python::object logCallback;
	boost::python::object codeCompleteCallback;
	Clara::DiagnosticConsumer* diagnosticConsumer = nullptr;
	std::string filename;
	std::vector<std::string> systemHeaders;
	std::string builtinHeaders;
	std::string jsonCompileCommands;
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