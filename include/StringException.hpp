#pragma once

#include <stdexcept>
#include <iostream>

namespace Clara {

class StringException : public std::exception
{
public:

	template <class TString> StringException(TString&& message)
	: mMessage(std::forward<TString>(message))
	{
		/* Empty on purpose. */
	}

	StringException(const StringException& other) : mMessage(other.mMessage) {}
	StringException(StringException&& other) : mMessage(std::move(other.mMessage)) {}
	StringException& operator = (const StringException& other)
	{
		mMessage = other.mMessage;
		return *this;
	}
	StringException& operator = (StringException&& other)
	{
		mMessage = std::move(other.mMessage);
		return *this;
	}

	inline const char* what() const noexcept override
	{
		return mMessage.c_str();
	}

private:

	std::string mMessage;
	
};

} // namespace Clara