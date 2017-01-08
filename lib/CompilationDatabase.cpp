#include "CompilationDatabase.hpp"

using namespace Clara;

CompilationDatabase::CompilationDatabase(const std::string& directory)
{
	std::string errorMsg;
	mDatabase = clang::tooling::CompilationDatabase::autoDetectFromDirectory(directory, errorMsg);
	if (!mDatabase) throw std::runtime_error("failed to load compilation database.");
}

std::tuple<std::vector<std::string>, std::string> CompilationDatabase::operator[](const std::string& filename) const
{
	std::vector<std::string> result;
	if (!mDatabase) return std::make_tuple(std::move(result), std::string());
	const auto compileCommands = mDatabase->getCompileCommands(filename);
	if (compileCommands.empty()) return std::make_tuple(std::move(result), std::string());
	for (const auto& compileCommand : compileCommands)
	{
		// Skip first argument, because that's the path to the compiler.
		for (std::size_t i = 1; i < compileCommand.CommandLine.size(); ++i)
		{
			const auto& str = compileCommand.CommandLine[i];
			// if (str.find("-o") != std::string::npos)
			// {
			// 	++i;
			// }
			// else
			// {
				if (!str.empty()) result.emplace_back(str.c_str());
			// }
		}
	}
	return std::make_tuple(std::move(result), compileCommands[0].Directory);
}
