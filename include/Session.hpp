#pragma once

#include <boost/python/object.hpp>
#include <string>
#include <clang/Frontend/CompilerInstance.h>
#include <functional>
#include "CancellableSyntaxOnlyAction.hpp"

namespace Clara {

struct SessionOptions; // forward declaration.
class DiagnosticConsumer; // forward declaration.

/**
 * @brief      Encapsulates a single file session.
 */
class Session
{
public:

	boost::python::object reporter;

	/**
	 * @brief      Constructs a new session.
	 *
	 * @param[in]  options  The options
	 */
	Session(const SessionOptions& options);

	/**
	 * @brief      Constructs a new session from a filename, without compile commands.
	 *
	 * @param[in]  filename  The filename
	 */
	Session(const std::string& filename);

	/**
	 * @brief      Constructs a new session from a filename, with compile commands.
	 *
	 * @param[in]  filename             The filename
	 * @param[in]  compileCommandsJson  The directory that contains the "compile_commands.json" file.
	 */
	Session(const std::string& filename, const std::string& compileCommandsJson);

	/**
	 * @brief      Constructs a session with a custom diagnostic consumer.
	 *
	 * @param      consumer  The consumer
	 * @param[in]  filename  The filename
	 */
	Session(clang::DiagnosticConsumer& consumer, const std::string& filename);

	/**
	 * @brief      Constructs a session with a custom diagnostic consumer.
	 *
	 * @param      consumer             The consumer
	 * @param[in]  filename             The filename
	 * @param[in]  compileCommandsJson  The compile commands json
	 */
	Session(clang::DiagnosticConsumer& consumer, const std::string& filename, const std::string& compileCommandsJson);

	/**
	 * @brief      Constructs a single file session.
	 *
	 * @param      consumer  The consumer
	 * @param[in]  filename  The filename
	 */
	Session(Clara::DiagnosticConsumer& consumer, const std::string& filename);


	/**
	 * @brief      Constructs a single file sesssion with the given compile
	 *             commands.
	 *
	 * @param      consumer             The consumer
	 * @param[in]  filename             The filename
	 * @param[in]  compileCommandsJson  The compile commands in JSON format spit
	 *                                  out by CMake.
	 */
	Session(Clara::DiagnosticConsumer& consumer, const std::string& filename, const std::string& compileCommandsJson);


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
	std::vector<std::pair<std::string, std::string>> codeComplete(const char* unsavedBuffer, int row, int column);

	/**
	 * @brief      Asynchronous version of Session::codeComplete.
	 *
	 * @param[in]  unsavedBuffer  The buffer that is not yet saved to disk.
	 * @param[in]  row            Clang rows are 1-based, not 0-based.
	 * @param[in]  column         Clang columns are 1-based, not 0-based.
	 * @param[in]  callback       A callable python object that receives a list of string pairs.
	 */
	void codeCompleteAsync(const char* unsavedBuffer, int row, int column, boost::python::object callback);


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

private:

	void setupBasicLangOptions(const SessionOptions& options);
	void tryLoadCompilationDatabase(const SessionOptions& options);

	std::string mFilename;

	clang::CompilerInstance mInstance;

	CancellableSyntaxOnlyAction mAction;
};

} // namespace Clara