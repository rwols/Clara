#pragma once

#include <clang/Tooling/CompilationDatabase.h>
#include <vector>
#include <string>
#include <memory>

namespace Clara {

class CompilationDatabase
{
public:
	CompilationDatabase(const std::string& directory);
	std::tuple<std::vector<std::string>, std::string> operator[] (const std::string& filename) const;
	operator bool() const noexcept;
private:
	std::unique_ptr<clang::tooling::CompilationDatabase> mDatabase;
};

} // namespace Clara