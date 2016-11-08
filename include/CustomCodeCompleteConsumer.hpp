#pragma once

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>
#include <boost/python.hpp>
#include <string>

class CustomCodeCompleteConsumer : public clang::CodeCompleteConsumer 
{
public:

	bool includeOptionalArguments = true;

	CustomCodeCompleteConsumer(const clang::CodeCompleteOptions& options);

	void ProcessCodeCompleteResults(
		clang::Sema &sema, 
		clang::CodeCompletionContext context,
		clang::CodeCompletionResult* results,
		unsigned numResults) override;

	void ProcessOverloadCandidates(
		clang::Sema &sema,
		unsigned currentArg,
		clang::CodeCompleteConsumer::OverloadCandidate* candidates,
		unsigned numCandidates) override;

	clang::CodeCompletionAllocator& getAllocator() override;

	clang::CodeCompletionTUInfo& getCodeCompletionTUInfo() override;

	inline boost::python::list getPythonResultList() const
	{
		return mResultList;
	}

private:

	clang::CodeCompletionTUInfo CCTUInfo;

	boost::python::list mResultList;

	boost::python::list ProcessCodeCompleteResult(
		clang::Sema& sema,
		clang::CodeCompletionContext context,
		clang::CodeCompletionResult& result);

	void ProcessCodeCompleteString(
		const clang::CodeCompletionString& ccs, 
		unsigned& argCount,
		std::string& first, 
		std::string& second,
		std::string& informative) const;

	void printCompletionResult(std::ostream& os, clang::CodeCompletionResult* result);
};