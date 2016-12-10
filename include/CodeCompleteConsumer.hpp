#pragma once

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>
#include <string>

namespace Clara {

class Session; // forward declaration

class CodeCompleteConsumer : public clang::CodeCompleteConsumer 
{
public:

	bool includeOptionalArguments = true;

	// clang::SmallVector<clang::StoredDiagnostic, 8> Diagnostics;
	// clang::SmallVector<CXStoredDiagnostic *, 8> DiagnosticsWrappers;
	// clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts;
	// clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> Diag;
	// clang::LangOptions LangOpts;
	// clang::IntrusiveRefCntPtr<clang::FileManager> FileMgr;
	// clang::IntrusiveRefCntPtr<clang::SourceManager> SourceMgr;
	// std::vector<std::string> TemporaryFiles;
	// clang::SmallVector<const llvm::MemoryBuffer *, 1> TemporaryBuffers;

	CodeCompleteConsumer(const clang::CodeCompleteOptions& options);
		// clang::IntrusiveRefCntPtr<clang::FileManager> fileManager, 
		// std::string filename, int row, int column);

    ~CodeCompleteConsumer() override = default;

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

	// std::string mFilename;
	// int mRow;
	// int mColumn;

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
