#pragma once

#include <string>

class Project
{
public:
	Project(const std::string& msg);
	void set(const std::string& msg);
	std::string greet() const noexcept;
private:
	std::string mMessage;
};