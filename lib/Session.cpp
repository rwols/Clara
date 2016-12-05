#include "Session.hpp"
#include "CodeCompleteConsumer.hpp"
#include "StringException.hpp"
#include "CancelException.hpp"
#include "DiagnosticConsumer.hpp"
#include "SessionOptions.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <clang/Basic/TargetInfo.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Frontend/FrontendActions.h>
#include <pybind11/stl.h>


#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

namespace Clara {

class SimplePrinter : public DiagnosticConsumer
{
public:
	SimplePrinter(Session& owner) : mOwner(owner) {}
	~SimplePrinter() override = default;
	void handleNote(const std::string& filename, int row, int column, const std::string& message) override
	{
		reportMsg("NOTE   ", filename, row, column, message);
	}
	void handleRemark(const std::string& filename, int row, int column, const std::string& message) override
	{
		reportMsg("REMARK ", filename, row, column, message);
	}
	void handleWarning(const std::string& filename, int row, int column, const std::string& message) override
	{
		reportMsg("WARNING", filename, row, column, message);
	}
	void handleError(const std::string& filename, int row, int column, const std::string& message) override
	{
		reportMsg("ERROR  ", filename, row, column, message);
	}
	void handleFatalError(const std::string& filename, int row, int column, const std::string& message) override
	{
		reportMsg("FATAL  ", filename, row, column, message);
	}
private:
	void reportMsg(const char* prefix, const std::string& filename, int row, int column, const std::string& message)
	{
		std::string msg(prefix);
		msg.append(": ");
		if (!filename.empty())
		{
			msg.append(filename);
			msg.append(":").append(std::to_string(row));
			msg.append(":").append(std::to_string(column));
			msg.append(": ");
		}
		msg.append(message);
		mOwner.report(msg.c_str());
	}
	Session& mOwner;
};

Session::Session(const SessionOptions& options)
: reporter(options.logCallback)
, mOptions(options)
, mFilename(options.filename)
, mAction(*this)
{
	using namespace clang;
	using namespace clang::tooling;
	resetDiagnosticsAndSourceManager();
	CodeCompleteOptions codeCompleteOptions;
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));
}

clang::CompilerInvocation* Session::makeInvocation() const
{
	using namespace clang;
	auto invocation = new CompilerInvocation();
	invocation->TargetOpts->Triple = llvm::sys::getDefaultTargetTriple();
	invocation->setLangDefaults(
		*invocation->getLangOpts(), 
		IK_CXX, 
		llvm::Triple(invocation->TargetOpts->Triple), 
		invocation->getPreprocessorOpts(), 
		mOptions.languageStandard);

	auto& frontendOpts = invocation->getFrontendOpts();
	frontendOpts.Inputs.emplace_back
	(
		mFilename,
		FrontendOptions::getInputKindForExtension(mFilename)
	);

	fillInvocationWithStandardHeaderPaths(invocation);

	return invocation;
}

void Session::fillInvocationWithStandardHeaderPaths(clang::CompilerInvocation* invocation) const
{
	auto& headerSearchOpts = invocation->getHeaderSearchOpts();

	#ifdef PRINT_HEADER_SEARCH_PATHS
		headerSearchOpts.Verbose = true;
	#else
		headerSearchOpts.Verbose = false;
	#endif

	headerSearchOpts.UseBuiltinIncludes = true;
	headerSearchOpts.UseStandardSystemIncludes = true;
	headerSearchOpts.UseStandardCXXIncludes = true;
	headerSearchOpts.ResourceDir = mOptions.builtinHeaders;

	for (const auto& systemHeader : mOptions.systemHeaders)
	{
		headerSearchOpts.AddPath(systemHeader, clang::frontend::System, false, false);
	}
}

void Session::loadFromOptions()
{
	using namespace clang;
	using namespace clang::tooling;

	CompilerInvocation* invocation;

	if (!mOptions.jsonCompileCommands.empty())
	{
		report("Attemping to load JSON compilation database.");
		std::string errorMsg;
		auto compdb = CompilationDatabase::autoDetectFromDirectory(mOptions.jsonCompileCommands, errorMsg);
		if (compdb)
		{
			auto compileCommands = compdb->getCompileCommands(mFilename);
			if (!compileCommands.empty())
			{
				std::vector<const char*> cstrings;
				for (const auto& compileCommand : compileCommands)
				{
					// Skip first argument, because that's the path to the compiler.
					for (std::size_t i = 1; i < compileCommand.CommandLine.size(); ++i)
					{
						const auto& str = compileCommand.CommandLine[i];
						if (str.find("-o") != std::string::npos)
						{
							++i;
						}
						else
						{
							cstrings.push_back(str.c_str());
							// report(cstrings.back());
						}
					}
				}
				auto& diagnostics = mInstance.getDiagnostics();
				invocation = createInvocationFromCommandLine(cstrings, &diagnostics);
				if (invocation)
				{
					invocation->getFileSystemOpts().WorkingDir = compileCommands[0].Directory;
					fillInvocationWithStandardHeaderPaths(invocation);
					report("Succesfully loaded compile commands.");
				}
				else
				{
					report("Compiler invocation failed.");
					invocation = makeInvocation();
				}
			}
			else
			{
				report("Could not find compile commands.");
				invocation = makeInvocation();
			}
		}
		else
		{
			report(errorMsg.c_str());
			invocation = makeInvocation();
		}
	}
	else
	{
		report("No JSON compilation database specified.");
		invocation = makeInvocation();
	}

	mInstance.setInvocation(invocation);
}

void Session::codeCompletePrepare(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	report("Preparing completion run.");
	mAction.cancel(); // Blocks until a concurrently running action has stopped.
	loadFromOptions();
	resetDiagnosticsAndSourceManager();
	auto& frontendOptions = mInstance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = mFilename;
	frontendOptions.CodeCompletionAt.Line = row;
	frontendOptions.CodeCompletionAt.Column = column;
	llvm::MemoryBufferRef bufferAsRef(unsavedBuffer, mFilename);
	auto memBuffer = llvm::MemoryBuffer::getMemBuffer(bufferAsRef);
	auto& preprocessorOptions = mInstance.getPreprocessorOpts();
	for (auto& remappedFile : preprocessorOptions.RemappedFileBuffers)
	{
		delete remappedFile.second;
	}
	preprocessorOptions.RemappedFileBuffers.clear();
	preprocessorOptions.RemappedFileBuffers.emplace_back(mFilename, memBuffer.release());
}

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	pybind11::gil_scoped_release release;
	codeCompletePrepare(unsavedBuffer, row, column);
	std::vector<std::pair<std::string, std::string>> result;
	if (mInstance.ExecuteAction(mAction))
	{
		auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
		consumer->moveResult(result);
	}
	else
	{
		report("Completion run FAILED.");
	}
	return result;
}

void Session::codeCompleteAsync(const char* unsavedBuffer, int row, int column, pybind11::object callback)
{
	using namespace clang;
	codeCompletePrepare(unsavedBuffer, row, column);
	std::thread task( [=] () -> void
	{
		try
		{
			if (mInstance.ExecuteAction(mAction))
			{
				auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
				std::vector<std::pair<std::string, std::string>> result;
				consumer->moveResult(result);
				pybind11::gil_scoped_acquire acquire;
				try
				{
					callback(row, column, result);
				}
				catch (const pybind11::cast_error& e)
				{
					reporter(e.what());
				}
				catch (const pybind11::error_already_set& e)
				{
					reporter(e.what());
				}
			}
			else
			{
				report("Completion run FAILED.");
			}
		}
		catch (const CancelException& /*exception*/)
		{
			report("Async completion was cancelled.");
		}

	});
	task.detach();
}

void Session::resetDiagnosticsAndSourceManager()
{
	mInstance.setSourceManager(nullptr);
	mInstance.createDiagnostics();
	auto client = new SimplePrinter(*this);

	// Takes ownership of the pointer so we don't have
	// to keep it around.
	mInstance.getDiagnostics().setClient(client);
}

const std::string& Session::getFilename() const noexcept
{
	return mFilename;
}

void Session::cancelAsyncCompletion()
{
	mAction.cancel();
}

void Session::report(const char* message)
{
	if (message == nullptr) return;
	pybind11::gil_scoped_acquire acquire;
	if (reporter != pybind11::object())
	{
		try
		{
			reporter(std::string(message));
		}
		catch (const pybind11::cast_error& e)
		{
			reporter(e.what());
		}
		catch (const pybind11::error_already_set& e)
		{
			reporter(e.what());
		}
	}
}

Session::~Session()
{
	mInstance.clearOutputFiles(false);
}

} // namespace Clara