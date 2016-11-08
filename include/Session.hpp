#pragma once

#include <string>
#include <vector>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/ASTUnit.h>
#include <boost/python.hpp>

class Session
{
public:
	Session() = default;
	std::string loadCompilationDatabase(const std::string& compilationDatabaseFile);
	bool hasCompilationDatabase() const noexcept;
	void setSourcePaths(const boost::python::list& sourcePaths);
	void reloadTool();
	boost::python::list codeComplete(const std::string& filename, const char* buffer, int row, int column);
private:
	std::vector<std::unique_ptr<clang::ASTUnit>> mSyntaxTrees;
	std::vector<std::string> mSourcePaths;
	std::unique_ptr<clang::tooling::CompilationDatabase> mCompilationDatabase;
	std::unique_ptr<clang::tooling::ClangTool> mTool;
};