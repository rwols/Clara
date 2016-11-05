#include "Project.hpp"

Project::Project(const std::string& msg)
: mMessage(msg)
{
	// empty
}

void Project::set(const std::string& msg)
{
	mMessage = msg;
}

std::string Project::greet() const noexcept
{
	return mMessage;
}