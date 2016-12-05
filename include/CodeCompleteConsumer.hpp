#pragma once

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>
#include <boost/python/list.hpp>
#include <string>

namespace Clara {

class Session; // forward declaration

class CodeCompleteConsumer : public clang::CodeCompleteConsumer 
{
public:

	bool includeOptionalArguments = true;

	CodeCompleteConsumer(const clang::CodeCompleteOptions& options, const Session& owner);

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

	void moveResult(std::vector<std::pair<std::string, std::string>>& result);

	void clearResult();

private:

	const Session& mOwner;

	clang::CodeCompletionTUInfo mCCTUInfo;

	std::vector<std::pair<std::string, std::string>> mResultList;

	std::pair<std::string, std::string> ProcessCodeCompleteResult(
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