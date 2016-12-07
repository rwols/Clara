#pragma once

#include "SessionOptions.hpp"
#include <string>
#include <clang/Frontend/CompilerInstance.h>
#include <functional>
#include "CancellableSyntaxOnlyAction.hpp"
#include "DiagnosticConsumer.hpp"

namespace Clara {

class DiagnosticConsumer; // forward declaration.

/**
 * @brief      Encapsulates a single file session.
 */
class Session
{
public:

	// pybind11::object reporter;

	/**
	 * @brief      Constructs a new session.
	 *
	 * @param[in]  options  The options
	 */
	Session(const SessionOptions& options);

	/**
	 * @brief      Destroys the object.
	 */
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
	std::vector<std::pair<std::string, std::string>> codeComplete(const char* unsavedBuffer, int row, int column) const;

	/**
	 * @brief      Asynchronous version of Session::codeComplete.
	 *
	 * @param[in]  unsavedBuffer  The buffer that is not yet saved to disk.
	 * @param[in]  row            Clang rows are 1-based, not 0-based.
	 * @param[in]  column         Clang columns are 1-based, not 0-based.
	 * @param[in]  callback       A callable python object that receives a list of string pairs.
	 */
	void codeCompleteAsync(std::string unsavedBuffer, int row, int column, pybind11::object callback) const;


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
	const std::string& getFilename() const noexcept;

	void report(const char* message) const;

private:

	friend class CancellableSyntaxOnlyAction;
	friend class CodeCompleteConsumer;

	clang::DiagnosticConsumer* createDiagnosticConsumer() const;
	void loadFromOptions(clang::CompilerInstance& instance) const;
	clang::CompilerInvocation* makeInvocation() const;
	void dump();
	void codeCompletePrepare(clang::CompilerInstance& instance, const char* unsavedBuffer, int row, int column) const;
	void fillInvocationWithStandardHeaderPaths(clang::CompilerInvocation* invocation) const;

	SessionOptions mOptions;
	Clara::DiagnosticConsumer mDiagConsumer;
};

} // namespace Clara