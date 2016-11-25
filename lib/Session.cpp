#include "Session.hpp"
#include "CodeCompleteConsumer.hpp"
#include "StringException.hpp"
#include "CancelException.hpp"
#include "DiagnosticConsumer.hpp"
#include "SessionOptions.hpp"
#include "PythonGILEnsurer.hpp"
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


#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

namespace Clara {

class PythonGILReleaser
{
	PyThreadState* mState;

public:

	PythonGILReleaser() : mState(PyEval_SaveThread()) {}

	~PythonGILReleaser() 
	{
		PyEval_RestoreThread(mState);
	}
};

// const char* standardIncludes[] =
// {
// 	"/usr/include/c++/5.4.0",
// 	"/usr/include/x86_64-linux-gnu/c++/5.4.0",
// 	"/usr/include/c++/5.4.0/backward",
// 	"/usr/local/include",
// 	"/usr/local/lib/clang/4.0.0/include",
// 	"/usr/include/x86_64-linux-gnu",
// 	"/usr/include"
// };

Session::Session(const SessionOptions& options)
: reporter(options.logCallback)
, mOptions(options)
, mFilename(options.filename)
, mAction(*this)
{
	using namespace clang;
	using namespace clang::tooling;
	mInstance.createDiagnostics();
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
		headerSearchOpts.AddPath(systemHeader, frontend::System, false, false);
	}

	return invocation;
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
					for (const auto& str : compileCommand.CommandLine) cstrings.push_back(str.c_str());
				}
				auto& diagnostics = mInstance.getDiagnostics();
				invocation = createInvocationFromCommandLine(cstrings, &diagnostics);
				if (invocation)
				{
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
	mInstance.setSourceManager(nullptr);
	mInstance.createDiagnostics();
	auto& frontendOptions = mInstance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = mFilename;
	frontendOptions.CodeCompletionAt.Line = row;
	frontendOptions.CodeCompletionAt.Column = column;
	llvm::MemoryBufferRef bufferAsRef(unsavedBuffer, mFilename);
	auto memBuffer = llvm::MemoryBuffer::getMemBuffer(bufferAsRef);
	auto& preprocessorOptions = mInstance.getPreprocessorOpts();
	preprocessorOptions.RemappedFileBuffers.clear();
	preprocessorOptions.RemappedFileBuffers.emplace_back(mFilename, memBuffer.release());
}

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	PythonGILReleaser guard;
	codeCompletePrepare(unsavedBuffer, row, column);
	std::vector<std::pair<std::string, std::string>> result;
	if (mInstance.ExecuteAction(mAction))
	{
		auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
		consumer->moveResult(result);
	}
	report("Finished completion run");
	return result;
}

void Session::codeCompleteAsync(const char* unsavedBuffer, int row, int column, pybind11::object callback)
{
	using namespace clang;
	PythonGILReleaser guard;
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
				PythonGILEnsurer pythonLock;
				callback(result);
			}
		}
		catch (const CancelException& /*exception*/)
		{
			report("Async completion was cancelled.");
		}

	});
	task.detach();
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
	PythonGILEnsurer lock;
	if (reporter != pybind11::object()) reporter(message);
}

Session::~Session()
{
	mInstance.clearOutputFiles(false);
}

} // namespace Clara