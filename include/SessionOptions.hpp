#pragma once

#include "PyBind11.hpp"
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/LangStandard.h>
#include <iosfwd>
#include <string>
#include <vector>

namespace Clara
{

class DiagnosticConsumer;

/**
 * @brief      A structure passed to a Session object
 */
struct SessionOptions
{
    clang::IntrusiveRefCntPtr<clang::FileManager> fileManager;
    pybind11::object logCallback;
    pybind11::object codeCompleteCallback;
    pybind11::object diagnosticCallback;
    std::string filename;
    std::vector<std::string> systemHeaders;
    std::vector<std::string> frameworks;
    std::string builtinHeaders;
    std::string jsonCompileCommands;
    std::string workingDirectory;
    std::string astFile;
    std::vector<std::string> invocation;
    clang::LangStandard::Kind languageStandard;
    bool codeCompleteIncludeMacros = true;
    bool codeCompleteIncludeCodePatterns = true;
    bool codeCompleteIncludeGlobals = true;
    bool codeCompleteIncludeBriefComments = true;
    bool codeCompleteIncludeOptionalArguments = true;
};

/**
 * @brief      Debug output for a SessionOptions object.
 *
 * @param      os       The output stream.
 * @param[in]  options  The object.
 *
 * @return     The modified output stream.
 */
std::ostream &operator<<(std::ostream &os, const SessionOptions &options);

} // namespace Clara
