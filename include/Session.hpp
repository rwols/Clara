#pragma once

#include "CancellableSyntaxOnlyAction.hpp"
#include "DiagnosticConsumer.hpp"
#include <string>
#include <clang/Frontend/CompilerInstance.h>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <functional>

namespace Clara {

/**
 * @brief      Encapsulates a single file session.
 */
class Session
{
public:

	boost::python::object reporter;

	Session(const std::string& filename);

	Session(const std::string& filename, const std::string& compileCommandsJson);

	Session(clang::DiagnosticConsumer& consumer, const std::string& filename);

	Session(clang::DiagnosticConsumer& consumer, const std::string& filename, const std::string& compileCommandsJson);

	/**
	 * @brief      Constructs a single file session.
	 *
	 * @param[in]  filename  The filename
	 */
	Session(Clara::DiagnosticConsumer& consumer, const std::string& filename);


	/**
	 * @brief      Constructs a single file sesssion with the given compile commands.
	 *
	 * @param[in]  filename             The filename
	 * @param[in]  compileCommandsJson  The compile commands in JSON format 
	 *                                  spit out by CMake.
	 */
	Session(Clara::DiagnosticConsumer& consumer, const std::string& filename, const std::string& compileCommandsJson);

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

	void codeCompleteAsync(const char* unsavedBuffer, int row, int column, boost::python::object callback);

	/**
	 * @brief      Returns the filename for the current session.
	 * 
	 * @return     The filename for the current session.
	 */
	const std::string& getFilename() const noexcept;

	void testAsync(boost::python::object callback);

private:

	std::string mFilename;

	clang::CompilerInstance mInstance;

	CancellableSyntaxOnlyAction mAction;
};

} // namespace Clara