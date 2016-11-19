#include "Session.hpp"
//#include "RenameFunctionFrontendActionFactory.hpp"
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
//#include <llvm/Support/CommandLine.h>
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

void Session::setupBasicLangOptions(const SessionOptions& options)
{
	// Setup language options
	auto &langOptions = mInstance.getLangOpts();
	langOptions.CPlusPlus = true;
	langOptions.CPlusPlus11 = options.cxx11;
	langOptions.CPlusPlus14 = options.cxx14;
	langOptions.CPlusPlus1z = options.cxx14;
}

Session::Session(const SessionOptions& options)
: reporter(options.logCallback)
, mFilename(options.filename)
, mAction(*this)
{
	using namespace clang;
	using namespace clang::tooling;
	using namespace boost;

	// Setup diagnostics engine
	mInstance.createDiagnostics();
	
	{
		PythonGILEnsurer lock;
		if (reporter != python::object())
		{
			reporter("Attemping to load JSON compilation database.");
		}
	}

	if (!options.jsonCompileCommands.empty())
	{
		std::string errorMsg;
		auto compdb = CompilationDatabase::autoDetectFromDirectory(options.jsonCompileCommands, errorMsg);
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
				auto invocation = std::unique_ptr<CompilerInvocation>(
					createInvocationFromCommandLine(cstrings, &diagnostics));
				if (invocation)
				{
					PythonGILEnsurer lock;
					if (reporter != python::object()) reporter("Succesfully loaded compile commands.");
					mInstance.setInvocation(invocation.release());
				}
				else
				{
					PythonGILEnsurer lock;
					if (reporter != python::object()) reporter("Compiler invocation failed.");
					setupBasicLangOptions(options);
				}
			}
			else
			{
				PythonGILEnsurer lock;
				if (reporter != python::object())
				{
					reporter("Could not find compile commands.");
				}
				setupBasicLangOptions(options);
			}
		}
		else
		{
			PythonGILEnsurer lock;
			if (reporter != python::object())
			{
				reporter("Could not load JSON compilation database: " + errorMsg);
			}
			setupBasicLangOptions(options);
		}
	}
	else
	{
		setupBasicLangOptions(options);
	}

	// Setup target, filemanager and sourcemanager
	auto targetOptions = std::make_shared<TargetOptions>();
	targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
	mInstance.setTarget(TargetInfo::CreateTargetInfo(mInstance.getDiagnostics(), targetOptions));
	mInstance.createFileManager();
	mInstance.createSourceManager(mInstance.getFileManager());

	// Create code completion consumer
	CodeCompleteOptions codeCompleteOptions;
	codeCompleteOptions.IncludeMacros = options.codeCompleteIncludeMacros;
	codeCompleteOptions.IncludeCodePatterns = options.codeCompleteIncludeCodePatterns;
	codeCompleteOptions.IncludeGlobals = options.codeCompleteIncludeGlobals;
	codeCompleteOptions.IncludeBriefComments = options.codeCompleteIncludeBriefComments;
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));

	// Setup header search paths
	mInstance.createPreprocessor(TranslationUnitKind::TU_Complete);
	auto& preprocessor = mInstance.getPreprocessor();
	auto& headerSearch = preprocessor.getHeaderSearchInfo();
	auto& headerSearchOpts = mInstance.getHeaderSearchOpts();
	headerSearchOpts.ResourceDir = options.builtinHeaders;
	std::vector<clang::DirectoryLookup> lookups;
	auto& fileManager = mInstance.getFileManager();
	for (const auto standardInclude : options.systemHeaders)
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

	CompilerInvocation::setLangDefaults(mInstance.getLangOpts(), IK_CXX, llvm::Triple(targetOptions->Triple), mInstance.getPreprocessorOpts());
}

Session::Session(const std::string& filename)
: mFilename(filename)
, mAction(*this)
{
	PythonGILReleaser releaser;

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
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));

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
, mAction(*this)
{
	PythonGILReleaser releaser;

	// DEBUG_PRINT;
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

	// DEBUG_PRINT;
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
	codeCompleteOptions.IncludeMacros = false;
	codeCompleteOptions.IncludeCodePatterns = true;
	codeCompleteOptions.IncludeGlobals = false;
	codeCompleteOptions.IncludeBriefComments = false;
	mInstance.setCodeCompletionConsumer(new Clara::CodeCompleteConsumer(codeCompleteOptions, *this));

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
	// DEBUG_PRINT;
	mInstance.getDiagnostics().setClient(&diagConsumer, false);
}

Session::Session(clang::DiagnosticConsumer& diagConsumer, const std::string& filename, const std::string& compileCommandsJson)
: Session(filename, compileCommandsJson)
{
	// DEBUG_PRINT;
	mInstance.getDiagnostics().setClient(&diagConsumer, false);
}

Session::Session(Clara::DiagnosticConsumer& diagConsumer, const std::string& filename)
: Session(static_cast<clang::DiagnosticConsumer&>(diagConsumer), filename)
{
	// DEBUG_PRINT;
	/* Empty on purpose. */
}

Session::Session(Clara::DiagnosticConsumer& diagConsumer, const std::string& filename, const std::string& compileCommandsJson)
: Session(static_cast<clang::DiagnosticConsumer&>(diagConsumer), filename, compileCommandsJson) // Delegating constructor, does all the things above here.
{
	// DEBUG_PRINT;
	/* Empty on purpose. */
}

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	using namespace boost;

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
		PythonGILEnsurer lock;
		if (reporter != python::object()) reporter("Preparing completion run");
	}

	if (mAction.BeginSourceFile(mInstance, frontendOptions.Inputs[0]))
	{
		{
			PythonGILEnsurer lock;
			if (reporter != python::object()) reporter("Executing syntax only action");
		}
		mAction.Execute();
		{
			PythonGILEnsurer lock;
			if (reporter != python::object()) reporter("Finishing syntax only action");
		}
		mAction.EndSourceFile();
		{
			PythonGILEnsurer lock;
			if (reporter != python::object()) reporter("Moving results");
		}
		auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
		consumer->moveResult(result);
	}
	{
		PythonGILEnsurer lock;
		if (reporter != python::object()) reporter("Finished completion run");
	}
	return result;
}

void Session::codeCompleteAsync(const char* unsavedBuffer, int row, int column, boost::python::object callback)
{
	using namespace clang;
	using namespace boost;

	mAction.cancel();

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
	
	if (mAction.BeginSourceFile(mInstance, frontendOptions.Inputs[0]))
	{
		std::thread task( [=] () -> void
		{
			try
			{
				std::vector<std::pair<std::string, std::string>> result;
				mAction.Execute();
				mAction.EndSourceFile();
				auto consumer = static_cast<Clara::CodeCompleteConsumer*>(&mInstance.getCodeCompletionConsumer());
				consumer->moveResult(result);
				PythonGILEnsurer pythonLock;
				callback(result);
			}
			catch (const CancelException& e)
			{
				mAction.EndSourceFile();
			}
		});
		task.detach();
	}
}

const std::string& Session::getFilename() const noexcept
{
	return mFilename;
}

void Session::cancelAsyncCompletion()
{
	mAction.cancel();
}

// bool gPleaseCancel(false);
// bool gIsRunning(false);
// std::mutex gCancelMutex;
// std::condition_variable gVar;

// void Session::testAsync(boost::python::object callback)
// {
//  std::unique_lock<std::mutex> cancelLock(gCancelMutex);
//  if (gIsRunning)
//  {
//      callback("Task is already running. Waiting...");
//      gPleaseCancel = true;
//      gVar.wait(cancelLock, []{ return !gIsRunning; });
//      callback("Done!");
//  }
//  else
//  {
//      callback("Nothing is running.");
//  }
//  cancelLock.unlock();
//  std::thread task([=] () -> void
//  {
//      std::unique_lock<std::mutex> cancelLock(gCancelMutex);
//      gIsRunning = true;
//      cancelLock.unlock();
//      try
//      {
//          for (int i = 0; i < 30; ++i)
//          {
//              std::this_thread::sleep_for(std::chrono::milliseconds(100));
//              cancelLock.lock();
//              if (gPleaseCancel)
//              {
//                  cancelLock.unlock();
//                  throw std::exception();
//              }
//              cancelLock.unlock();
//          }
//          PythonGILEnsurer pythonLock;
//          callback("Finished with long task.");
//      }
//      catch (const std::exception& e)
//      {

//      }
//      cancelLock.lock();
//      gPleaseCancel = false;
//      gIsRunning = false;
//      cancelLock.unlock();
//      gVar.notify_all();
//  });
//  task.detach(); // bye bye!
// }

Session::~Session()
{
	mInstance.clearOutputFiles(false);
}

} // namespace Clara