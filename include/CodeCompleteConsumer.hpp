#pragma once

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>
#include <boost/python.hpp>
#include <string>

namespace Clara {

class CodeCompleteConsumer : public clang::CodeCompleteConsumer 
{
public:

	bool includeOptionalArguments = true;

	CodeCompleteConsumer(const clang::CodeCompleteOptions& options);

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

	boost::python::list getPythonResultList() const noexcept;

private:

	clang::CodeCompletionTUInfo mCCTUInfo;

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

} // namespace Clara