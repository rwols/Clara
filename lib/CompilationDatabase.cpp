#include "CompilationDatabase.hpp"

using namespace Clara;

CompilationDatabase::CompilationDatabase(const std::string &directory)
{
    std::string errorMsg;
    mDatabase = clang::tooling::CompilationDatabase::autoDetectFromDirectory(
        directory, errorMsg);
    if (!mDatabase)
        throw std::runtime_error("failed to load compilation database.");
    if (!errorMsg.empty())
    {
        throw std::runtime_error(errorMsg);
    }
}

std::tuple<std::vector<std::string>, std::string> CompilationDatabase::
operator[](const std::string &filename) const
{
    std::vector<std::string> result;
    if (!mDatabase) return std::make_tuple(std::move(result), std::string());
    const auto compileCommands = mDatabase->getCompileCommands(filename);
    if (compileCommands.empty())
        return std::make_tuple(std::move(result), std::string());

    // FIXME: What to do in case there are multiple (different) compile commands
    // for the same translation unit? Right now we take the first that we
    // encounter. This fails to give correct completion results when there are
    // different #define's on the command line for the different invocations,
    // just to give an example.
    const auto &compileCommand = compileCommands[0];
    // Skip first argument, because that's the path to the compiler.
    for (std::size_t i = 1; i < compileCommand.CommandLine.size(); ++i)
    {
        const auto &str = compileCommand.CommandLine[i];
        if (!str.empty()) result.emplace_back(str.c_str());
    }
    return std::make_tuple(std::move(result), compileCommands[0].Directory);
}
