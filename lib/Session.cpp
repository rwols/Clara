#include "Session.hpp"
#include <thread>
#include <mutex>
#include <llvm/Support/CommandLine.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Basic/TargetInfo.h>
#include "RenameFunctionFrontendActionFactory.hpp"
#include "CodeCompleteConsumer.hpp"
#include "StringException.hpp"
#include "CancelException.hpp"

#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

namespace Clara {

class PythonGILEnsurer 
{
	PyGILState_STATE mState;

public:
	
	PythonGILEnsurer() : mState(PyGILState_Ensure()) {}

	~PythonGILEnsurer() 
	{
		PyGILState_Release(mState);
	}
};

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

const char* standardIncludes[] =
{
	"/usr/include/c++/5.4.0",
	"/usr/include/x86_64-linux-gnu/c++/5.4.0",
	"/usr/include/c++/5.4.0/backward",
	"/usr/local/include",
	"/usr/local/lib/clang/4.0.0/include",
	"/usr/include/x86_64-linux-gnu",
	"/usr/include"
};

Session::Session(const std::string& filename)
: mFilename(filename)
{
	PythonGILReleaser releaser;

	DEBUG_PRINT;
	using namespace clang;
	using namespace boost;

	// Setup language options
	auto &langOptions = mInstance.getLangOpts();
	langOptions.CPlusPlus = true;
	langOptions.CPlusPlus11 = true;

	// Setup diagnostics engine
	mInstance.createDiagnostics();

	// Setup target, filemanager and sourcemanager
	auto targetOptions = std::make_shared<TargetOptions>();
	targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
	mInstance.setTarget(TargetInfo::CreateTargetInfo(mInstance.getDiagnostics(), targetOptions));
	mInstance.createFileManager();
	mInstance.createSourceManager(mInstance.getFileManager());

	// Create code completion consumer
	CodeCompleteOptions codeCompleteOptions;
	codeCompleteOptions.IncludeMacros = true;
	codeCompleteOptions.IncludeCodePatterns = true;
	codeCompleteOptions.IncludeGlobals = true;
	codeCompleteOptions.IncludeBriefComments = true;
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions));

	// Setup header search paths
	mInstance.createPreprocessor(TranslationUnitKind::TU_Complete);
	auto& preprocessor = mInstance.getPreprocessor();
	auto& headerSearch = preprocessor.getHeaderSearchInfo();
	auto& headerSearchOpts = mInstance.getHeaderSearchOpts();
	headerSearchOpts.ResourceDir = "/usr/local/lib/clang/4.0.0/include/";
	std::vector<clang::DirectoryLookup> lookups;
	auto& fileManager = mInstance.getFileManager();
	for (const auto standardInclude : standardIncludes)
	{
		headerSearchOpts.UserEntries.emplace_back(standardInclude, frontend::IncludeDirGroup::System, false, false);
		lookups.emplace_back(fileManager.getDirectory(standardInclude), SrcMgr::CharacteristicKind::C_System, false);
	}
	headerSearch.SetSearchPaths(lookups, 0, 0, true);

	// Create frontend options
	auto& frontendOptions = mInstance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = mFilename;
	FrontendInputFile input(mFilename, InputKind::IK_CXX);
	frontendOptions.Inputs.push_back(input);

	CompilerInvocation::setLangDefaults(langOptions, IK_CXX, llvm::Triple(targetOptions->Triple), mInstance.getPreprocessorOpts());
}

Session::Session(const std::string& filename, const std::string& compileCommandsJson)
: mFilename(filename)
{
	PythonGILReleaser releaser;

	DEBUG_PRINT;
	using namespace clang;
	using namespace clang::tooling;
	using namespace boost;

	// Setup diagnostics engine
	mInstance.createDiagnostics();

	std::string errorMsg;
	auto compdb = CompilationDatabase::autoDetectFromDirectory(compileCommandsJson, errorMsg);
	if (!compdb) throw StringException(std::move(errorMsg));
	
	auto compileCommands = compdb->getCompileCommands(mFilename);
	if (compileCommands.empty())
	{
		throw StringException("Could not find compile commands for " + mFilename);
	}
	std::vector<const char*> cstrings;
	for (const auto& compileCommand : compileCommands)
	{
		for (const auto& str : compileCommand.CommandLine)
		{
			cstrings.push_back(str.c_str());
		}
	}

	auto& diagnostics = mInstance.getDiagnostics();

	auto invocation = std::unique_ptr<CompilerInvocation>(createInvocationFromCommandLine(cstrings, &diagnostics));
	if (!invocation)
	{
		errorMsg = "Failed to create a valid compiler instance from the given command line arguments for ";
		errorMsg += mFilename;
		errorMsg += ". They were: ";
		for (std::size_t i = 0; i < cstrings.size(); ++i)
		{
			errorMsg += cstrings[i];
			if (i != cstrings.size() - 1)
			{
				errorMsg += " ";
			}
		}
		throw StringException(std::move(errorMsg));
	}

	mInstance.setInvocation(invocation.release());

	DEBUG_PRINT;
	using namespace clang;
	using namespace boost;

	// Setup language options
	auto &langOptions = mInstance.getLangOpts();
	langOptions.CPlusPlus = true;
	langOptions.CPlusPlus11 = true;

	// Setup target, filemanager and sourcemanager
	auto targetOptions = std::make_shared<TargetOptions>();
	targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
	mInstance.setTarget(TargetInfo::CreateTargetInfo(mInstance.getDiagnostics(), targetOptions));
	mInstance.createFileManager();
	mInstance.createSourceManager(mInstance.getFileManager());

	// Create code completion consumer
	CodeCompleteOptions codeCompleteOptions;
	codeCompleteOptions.IncludeMacros = true;
	codeCompleteOptions.IncludeCodePatterns = true;
	codeCompleteOptions.IncludeGlobals = true;
	codeCompleteOptions.IncludeBriefComments = true;
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions));

	// Setup header search paths
	mInstance.createPreprocessor(TranslationUnitKind::TU_Complete);
	auto& preprocessor = mInstance.getPreprocessor();
	auto& headerSearch = preprocessor.getHeaderSearchInfo();
	auto& headerSearchOpts = mInstance.getHeaderSearchOpts();
	headerSearchOpts.ResourceDir = "/usr/local/lib/clang/4.0.0/include/";
	std::vector<clang::DirectoryLookup> lookups;
	auto& fileManager = mInstance.getFileManager();
	for (const auto standardInclude : standardIncludes)
	{
		headerSearchOpts.UserEntries.emplace_back(standardInclude, frontend::IncludeDirGroup::System, false, false);
		lookups.emplace_back(fileManager.getDirectory(standardInclude), SrcMgr::CharacteristicKind::C_System, false);
	}
	headerSearch.SetSearchPaths(lookups, 0, 0, true);

	// Create frontend options
	auto& frontendOptions = mInstance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = mFilename;
	FrontendInputFile input(mFilename, InputKind::IK_CXX);
	frontendOptions.Inputs.push_back(input);

	CompilerInvocation::setLangDefaults(langOptions, IK_CXX, llvm::Triple(targetOptions->Triple), mInstance.getPreprocessorOpts());
}

Session::Session(clang::DiagnosticConsumer& diagConsumer, const std::string& filename)
: Session(filename)
{
	DEBUG_PRINT;
	mInstance.getDiagnostics().setClient(&diagConsumer, false);
}

Session::Session(clang::DiagnosticConsumer& diagConsumer, const std::string& filename, const std::string& compileCommandsJson)
: Session(filename, compileCommandsJson)
{
	DEBUG_PRINT;
	mInstance.getDiagnostics().setClient(&diagConsumer, false);
}

Session::Session(Clara::DiagnosticConsumer& diagConsumer, const std::string& filename)
: Session(static_cast<clang::DiagnosticConsumer&>(diagConsumer), filename)
{
	DEBUG_PRINT;
	/* Empty on purpose. */
}

Session::Session(Clara::DiagnosticConsumer& diagConsumer, const std::string& filename, const std::string& compileCommandsJson)
: Session(static_cast<clang::DiagnosticConsumer&>(diagConsumer), filename, compileCommandsJson) // Delegating constructor, does all the things above here.
{
	DEBUG_PRINT;
	/* Empty on purpose. */
}

std::mutex gCompletionMutex;

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	using namespace boost;

	std::lock_guard<std::mutex> lock(gCompletionMutex);
	{
		std::stringstream ss;
		ss << "Acquired lock for thread " << std::this_thread::get_id();
		PythonGILEnsurer lock;
		reporter(ss.str());
	}

	auto& frontendOptions = mInstance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.Line = row;
	frontendOptions.CodeCompletionAt.Column = column;

	auto& preprocessorOptions = mInstance.getPreprocessorOpts();
	preprocessorOptions.RemappedFileBuffers.clear();
	if (unsavedBuffer != nullptr)
	{
		llvm::MemoryBufferRef bufferAsRef(unsavedBuffer, mFilename);
		auto memBuffer = llvm::MemoryBuffer::getMemBuffer(bufferAsRef);
		preprocessorOptions.RemappedFileBuffers.emplace_back(mFilename, memBuffer.release());
	}

	std::vector<std::pair<std::string, std::string>> result;

	{
		std::stringstream ss;
		ss << "Cancelling action on thread " << std::this_thread::get_id();
		PythonGILEnsurer lock;
		reporter(ss.str());
	}
	mAction.setCancelAtomic(false);
	if (mAction.BeginSourceFile(mInstance, frontendOptions.Inputs[0]))
	{
		{
			std::stringstream ss;
			ss << "Starting new execution on thread " << std::this_thread::get_id();
			PythonGILEnsurer lock;
			reporter(ss.str());
		}
		mAction.Execute();
		mAction.EndSourceFile();
		if (!mAction.isCancelledAtomic())
		{
			std::stringstream ss;
			ss << "Fetching results for thread " << std::this_thread::get_id();
			PythonGILEnsurer lock;
			reporter(ss.str());
			auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
			consumer->moveResult(result);
		}
		else
		{
			std::stringstream ss;
			ss << "Action was cancelled for thread " << std::this_thread::get_id();
			PythonGILEnsurer lock;
			reporter(ss.str());
		}
	}
	return result;
}

void Session::codeCompleteAsync(const char* unsavedBuffer, int row, int column, boost::python::object callback)
{
	using namespace clang;
	using namespace boost;
	std::string copyOfUnsavedBuffer(unsavedBuffer);

	auto lambda = [=] () -> void 
	{
		try
		{
			auto results = codeComplete(copyOfUnsavedBuffer.c_str(), row, column);	
			PythonGILEnsurer lock;
			if (mAction.isCancelledAtomic()) return;
			reporter("Completions are done. Calling callback.");
			callback(results);
		}
		catch (const CancelException& e)
		{
			// do nothing
		}
		catch (const std::exception& e)
		{
			PythonGILEnsurer lock;
			reporter("failed!");
		}
	};
	reporter("Code completing asynchronously at row " + std::to_string(row) + ", column " + std::to_string(column));
	mAction.setCancelAtomic(true);
	std::thread t(lambda);
	t.detach();
}

const std::string& Session::getFilename() const noexcept
{
	return mFilename;
}

void Session::testAsync(boost::python::object callback)
{
	auto lambda = [=] () -> void
	{
		{
			PythonGILEnsurer lock;
			callback("Starting a long task...");
		}

		std::this_thread::sleep_for(std::chrono::seconds(3));

		{
			PythonGILEnsurer lock;
			callback("Finished with long task.");
		}
	};

	// lambda();

	std::thread t(lambda);
	t.detach();
}

} // namespace Clara