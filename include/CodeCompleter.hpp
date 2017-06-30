#pragma once

#include "PyBind11.hpp"
#include <atomic>
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/PCHContainerOperations.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>
#include <condition_variable>
#include <future>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Clara
{

class CodeCompleter : public clang::DiagnosticConsumer,
                      public clang::CodeCompleteConsumer
{
  public:
    CodeCompleter(pybind11::object view);
    ~CodeCompleter() override;

    // clang::CodeCompleteConsumer implementation
    clang::CodeCompletionAllocator &getAllocator() override;
    clang::CodeCompletionTUInfo &getCodeCompletionTUInfo() override;
    void ProcessCodeCompleteResults(clang::Sema &sema,
                                    clang::CodeCompletionContext context,
                                    clang::CodeCompletionResult *results,
                                    unsigned numResults) override;
    void ProcessOverloadCandidates(
        clang::Sema &sema, unsigned currentArg,
        clang::CodeCompleteConsumer::OverloadCandidate *candidates,
        unsigned numCandidates) override;

    // clang::DiagnosticConsumer implementation
    void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                          const clang::Diagnostic &info) override;

    // Methods that will be exported to Python
    // is_applicable must be defined in python because it's a @classmethod.
    std::vector<std::pair<std::string, std::string>>
    onQueryCompletions(pybind11::str prefix, pybind11::list locations);
    void onPostSave();
    void reparse();

    static void registerClass(pybind11::module &m);

  private:
    void backgroundWorker();
    void initAST(std::vector<std::string> command,
                 std::vector<std::string> systemHeaders,
                 std::vector<std::string> systemFrameworks,
                 std::string builtinHeaders);
    void codeCompleteImpl();
    clang::CodeCompleteOptions initCodeCompleteOptions() const;
    void addPath(clang::CompilerInvocation *invocation, const std::string &path,
                 bool isFramework) const;
    std::pair<std::string, std::string>
    ProcessCodeCompleteResult(clang::Sema &sema,
                              clang::CodeCompletionContext context,
                              clang::CodeCompletionResult &result);

    void ProcessCodeCompleteString(const clang::CodeCompletionString &ccs,
                                   unsigned &argCount, std::string &first,
                                   std::string &second,
                                   std::string &informative) const;
    std::atomic_bool mIsLoaded{false};
    clang::CodeCompletionTUInfo mCCTUInfo;
    pybind11::object mView;
    clang::SmallVector<clang::StoredDiagnostic, 8>
        mStoredDiags; // ugly hack, wait for clang devs to fix this
    clang::SmallVector<const llvm::MemoryBuffer *, 1>
        mOwnedBuffers; // ugly hack, wait for clang devs to fix this
    std::shared_ptr<clang::PCHContainerOperations> mPchOps =
        std::make_shared<clang::PCHContainerOperations>();
    // SessionOptions mOptions;
    clang::LangOptions mLangOpts;
    clang::FileSystemOptions mFileOpts;
    clang::IntrusiveRefCntPtr<clang::FileManager> mFileMgr;
    clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> mDiagIds;
    clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> mDiagOpts;
    // Clara::DiagnosticConsumer mDiagConsumer;
    clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> mDiags;
    clang::IntrusiveRefCntPtr<clang::SourceManager> mSourceMgr;
    // std::unique_ptr<Clara::CodeCompleteConsumer> mCodeCompleteConsumer;
    std::unique_ptr<clang::ASTUnit> mUnit;
    unsigned mPoint;
    unsigned mRow;
    unsigned mColumn;
    std::string mUnsavedBuffer;
    bool mDoCompletionJob = false;
    std::string mFilename;
    std::vector<std::pair<std::string, std::string>> mCompletions;
    std::thread mInitThread;
    std::thread mWorkerThread;

    mutable std::mutex mMethodMutex;
    mutable std::condition_variable mConditionVar;
};

} // Clara
