#pragma once

#include "CodeCompleteConsumer.hpp"
#include "DiagnosticConsumer.hpp"
#include "SessionOptions.hpp"

#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/PCHContainerOperations.h>

#include <condition_variable>
#include <mutex>

namespace clang
{
class ASTUnit;
}

namespace Clara
{

class DiagnosticConsumer; // forward declaration.

/**
 * @brief      Encapsulates a single file session.
 */
class Session
{
  public:
    class ASTFileReadError : public std::exception
    {
      public:
        ~ASTFileReadError() noexcept override = default;
        const char *what() const noexcept override;
    };

    class ASTParseError : public std::exception
    {
      public:
        ~ASTParseError() noexcept override = default;
        const char *what() const noexcept override;
    };

    /**
     * @brief      Constructs a new session.
     *
     * @param[in]  options  The options
     */
    Session(const SessionOptions &options);

    ~Session();

    /**
     * @brief      Given a buffer that represents the unsaved file contents of
     *             the filename that was passed into the constructor, and given
     *             a row and column number, perform code completion at that row
     *             and column. The result can be passed immediately as-is to
     *             Sublime Text.
     *
     * @param[in]  unsavedBuffer  The buffer that is not yet saved to disk.
     * @param[in]  row            Clang rows are 1-based, not 0-based.
     * @param[in]  column         Clang columns are 1-based, not 0-based.
     *
     * @return     A python list which consists of pairs of strings.
     */
    std::vector<std::pair<std::string, std::string>>
    codeComplete(const char *unsavedBuffer, int row, int column);

    /**
     * @brief      Asynchronous version of Session::codeComplete.
     *
     * @param[in]  viewID         The view ID associated to the Sublime View
     * @param[in]  unsavedBuffer  The buffer that is not yet saved to disk.
     * @param[in]  row            Clang rows are 1-based, not 0-based.
     * @param[in]  column         Clang columns are 1-based, not 0-based.
     * @param[in]  callback       A callable python object that receives a list
     * of
     * string pairs.
     */
    void codeCompleteAsync(const int viewID, std::string unsavedBuffer, int row,
                           int column, pybind11::object callback);

    /**
     * @brief      Cancels the in-flight completion that is occurring in a
     *             background thread.
     */
    void cancelAsyncCompletion();

    /**
     * @brief      Returns the filename for the current session.
     *
     * @return     The filename for the current session.
     */
    const std::string &getFilename() const noexcept;

    /**
     * @brief      Reparse the AST, possibly recompiling the preamble.
     *
     * @return     True on success, false on failure.
     */
    bool reparse();

    /**
     * @brief      Saves the session to an abstract AST file in the build
     *             directory under the hidden subfolder ".clara".
     */
    void save() const;

    void report(const char *message) const;

  private:
    friend class CancellableSyntaxOnlyAction;
    friend class CodeCompleteConsumer;

    clang::DiagnosticConsumer *createDiagnosticConsumer() const;
    void loadFromOptions(clang::CompilerInstance &instance) const;
    std::unique_ptr<clang::CompilerInvocation> makeInvocation() const;
    void dump();
    void addPath(clang::CompilerInvocation *invocation,
                 const std::string &systemPath, bool isFramework) const;
    std::vector<std::pair<std::string, std::string>>
    codeCompleteImpl(const char *, int, int);
    void codeCompletePrepare(clang::CompilerInstance &instance,
                             const char *unsavedBuffer, int row,
                             int column) const;
    void fillInvocationWithStandardHeaderPaths(
        clang::CompilerInvocation *invocation) const;
    void resetDiagnosticsEngine();
    clang::CompilerInvocation *createInvocationFromOptions();

    clang::SmallVector<clang::StoredDiagnostic, 8>
        mStoredDiags; // ugly hack, wait for clang devs to fix this
    clang::SmallVector<const llvm::MemoryBuffer *, 1>
        mOwnedBuffers; // ugly hack, wait for clang devs to fix this
    std::shared_ptr<clang::PCHContainerOperations> mPchOps =
        std::make_shared<clang::PCHContainerOperations>();
    SessionOptions mOptions;
    clang::LangOptions mLangOpts;
    clang::FileSystemOptions mFileOpts;
    clang::IntrusiveRefCntPtr<clang::FileManager> mFileMgr;
    clang::DiagnosticIDs mDiagIds;
    clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> mDiagOpts;
    Clara::DiagnosticConsumer mDiagConsumer;
    clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> mDiags;
    clang::IntrusiveRefCntPtr<clang::SourceManager> mSourceMgr;
    std::unique_ptr<Clara::CodeCompleteConsumer> mCodeCompleteConsumer;
    std::unique_ptr<clang::ASTUnit> mUnit;

    int mViewID;
    int mRow;
    int mColumn;
    std::string mUnsavedBuffer;

    mutable std::mutex mMethodMutex;
    mutable std::condition_variable mConditionVar;
};

} // namespace Clara
