#pragma once

#include "Session.hpp"

namespace Clara {

class Project
{
public:
	Project() = default;
	Project(const std::string& JsonCompilationDatabaseFile);

	Session newViewIntoFile(const std::string& filename);
};

} // namespace Clara