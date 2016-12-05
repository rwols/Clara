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
// : reporter(options.logCallback)
: mOptions(options)
// , mFilename(options.filename)
// , mAction(*this)
{
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
		getFilename(),
		FrontendOptions::getInputKindForExtension(getFilename())
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

void Session::loadFromOptions(clang::CompilerInstance& instance) const
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
			auto compileCommands = compdb->getCompileCommands(getFilename());
			if (!compileCommands.empty())
			{
				std::vector<const char*> cstrings;
				for (const auto& compileCommand : compileCommands)
				{
					for (const auto& str : compileCommand.CommandLine) cstrings.push_back(str.c_str());
				}
				auto& diagnostics = instance.getDiagnostics();
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

	instance.setInvocation(invocation);
}

void Session::codeCompletePrepare(clang::CompilerInstance& instance, const char* unsavedBuffer, int row, int column) const
{
	using namespace clang;
	report("Preparing completion run.");
	// PythonGILReleaser guard;
	// mAction.cancel(); // Blocks until a concurrently running action has stopped.
	loadFromOptions(instance);
	instance.setSourceManager(nullptr);
	instance.createDiagnostics();
	auto& frontendOptions = instance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = getFilename();
	frontendOptions.CodeCompletionAt.Line = row;
	frontendOptions.CodeCompletionAt.Column = column;
	llvm::MemoryBufferRef bufferAsRef(unsavedBuffer, getFilename());
	auto memBuffer = llvm::MemoryBuffer::getMemBuffer(bufferAsRef);
	auto& preprocessorOptions = instance.getPreprocessorOpts();
	preprocessorOptions.RemappedFileBuffers.clear();
	preprocessorOptions.RemappedFileBuffers.emplace_back(getFilename(), memBuffer.release());
}

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column) const
{
	using namespace clang;
	using namespace clang::frontend;
	CompilerInstance instance;
	instance.createDiagnostics();
	CodeCompleteOptions codeCompleteOptions;
	instance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));
	codeCompletePrepare(instance, unsavedBuffer, row, column);
	std::vector<std::pair<std::string, std::string>> result;
	SyntaxOnlyAction action;
	if (instance.ExecuteAction(action))
	{
		auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&instance.getCodeCompletionConsumer());
		consumer->moveResult(result);
	}
	report("Finished completion run");
	return result;
}

void Session::codeCompleteAsync(std::string unsavedBuffer, int row, int column, pybind11::object callback) const
{
	using namespace clang;
	using namespace clang::frontend;
	
	std::thread task( [this, row, column, callback, unsavedBuffer{std::move(unsavedBuffer)} ] () -> void
	{
		CompilerInstance instance;
		instance.createDiagnostics();
		CodeCompleteOptions codeCompleteOptions;
		instance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));
		codeCompletePrepare(instance, unsavedBuffer.c_str(), row, column);
		std::vector<std::pair<std::string, std::string>> result;
		SyntaxOnlyAction action;
		if (instance.ExecuteAction(action))
		{
			auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&instance.getCodeCompletionConsumer());
			consumer->moveResult(result);
			PythonGILEnsurer pythonLock;
			callback(row, column, result);
		}

	});
	task.detach();
}

const std::string& Session::getFilename() const noexcept
{
	return mOptions.filename;
}

// void Session::cancelAsyncCompletion()
// {
// 	mAction.cancel();
// }

void Session::report(const char* message) const
{
	PythonGILEnsurer lock;
	if (message != nullptr && mOptions.logCallback != pybind11::object())
	{
		// yeah this is horrible.
		const_cast<Session*>(this)->mOptions.logCallback(message);
	}
}

Session::~Session()
{
	// mInstance.clearOutputFiles(false);
}

} // namespace Clara