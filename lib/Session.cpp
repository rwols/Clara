#include "../include/Session.hpp"
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/Utils.h> // for clang::createInvocationFromCommandLine
#include <pybind11/stl.h>

#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

using namespace Clara;

Session::Session(const SessionOptions& options)
: mOptions(options)
, mDiagOpts(new clang::DiagnosticOptions())
, mDiagConsumer(mOptions.diagnosticCallback)
, mDiags(new clang::DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer, false))
{
	using namespace clang;
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	pybind11::gil_scoped_release releaser;
	mFileOpts.WorkingDir = mOptions.workingDirectory;
	mFileMgr = new FileManager(mFileOpts);

	llvm::sys::fs::file_status astStatus, fileStatus;
	llvm::sys::fs::status(mOptions.filename, fileStatus);
	llvm::sys::fs::status(mOptions.astFile, astStatus);
	if (llvm::sys::fs::exists(astStatus))
	{
		if (astStatus.getLastModificationTime() >= fileStatus.getLastModificationTime())
		{
			std::string message("Reading \"");
			message.append(mOptions.astFile);
			message.append("\".");
			report(message.c_str());
			mUnit = clang::ASTUnit::LoadFromASTFile(
				mOptions.astFile, mPchOps->getRawReader(), mDiags, 
				mFileOpts, true, false, llvm::None, false, true, true);
		}
	}

	if (mUnit) return;

	clang::IntrusiveRefCntPtr<clang::CompilerInvocation> invocation(createInvocationFromOptions());
	mUnit = clang::ASTUnit::LoadFromCompilerInvocation(
		invocation.get(), 
		mPchOps, 
		mDiags, 
		mFileMgr.get(),
		/*OnlyLocalDecls*/ false, 
		/*CaptureDiagnostics*/ false,
		/*PrecompilePreambleAfterNParses*/ 2,
		/*TranslationUnitKind*/ clang::TU_Complete, 
		/*CacheCodeCompletionResults*/ true,
		/*IncludeBriefCommentsInCodeCompletion*/ mOptions.codeCompleteIncludeBriefComments, 
		/*UserFilesAreVolatile*/ true);
	if (mUnit->Reparse(mPchOps))
	{
		throw std::runtime_error("Failed to parse AST!");
	}
}

clang::CompilerInvocation* Session::createInvocationFromOptions()
{
	using namespace clang;
	CompilerInvocation* invocation = nullptr;
	if (!mOptions.invocation.empty())
	{
		std::vector<const char*> commandLine;
		for (const auto& str : mOptions.invocation) commandLine.push_back(str.c_str());
		invocation = createInvocationFromCommandLine(commandLine, mDiags);
		if (invocation)
		{
			invocation->getFileSystemOpts().WorkingDir = mOptions.workingDirectory;
			// mFileOpts.WorkingDir = mOptions.workingDirectory;
			fillInvocationWithStandardHeaderPaths(invocation);
			return invocation;
		}
	}
	else
	{
		invocation = makeInvocation();
	}
	return invocation;
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

	fillInvocationWithStandardHeaderPaths(invocation);

	return invocation;
}

void Session::fillInvocationWithStandardHeaderPaths(clang::CompilerInvocation* invocation) const
{
	auto& headerSearchOpts = invocation->getHeaderSearchOpts();
	invocation->getFrontendOpts().SkipFunctionBodies = 1;

	#ifdef PRINT_HEADER_SEARCH_PATHS
		headerSearchOpts.Verbose = true;
	#else
		headerSearchOpts.Verbose = false;
	#endif

	headerSearchOpts.UseBuiltinIncludes = true;
	headerSearchOpts.UseStandardSystemIncludes = true;
	headerSearchOpts.UseStandardCXXIncludes = true;

	// The resourcedir is hardcoded into the library now...
	// Don't see any other way on how to solve this.
	// headerSearchOpts.ResourceDir = mOptions.builtinHeaders;

	for (const auto& systemHeader : mOptions.systemHeaders)
	{
		headerSearchOpts.AddPath(systemHeader, clang::frontend::System, false, false);
	}
	for (const auto& framework : mOptions.frameworks)
	{
		headerSearchOpts.AddPath(framework, clang::frontend::System, true, false);
	}
}

void Session::resetDiagnosticsEngine()
{
	// IntrusiveRefCntPtr takes care of deleting things
	using namespace clang;
	mDiagOpts = new DiagnosticOptions();
	mDiags = new DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer, false);
	mSourceMgr = new SourceManager(*mDiags, *mFileMgr);
}

std::vector<std::pair<std::string, std::string>> Session::codeCompleteImpl(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	using namespace clang::frontend;
	SmallVector<ASTUnit::RemappedFile, 1> remappedFiles;
	auto memBuffer = llvm::MemoryBuffer::getMemBufferCopy(unsavedBuffer);
	remappedFiles.emplace_back(mOptions.filename, memBuffer.get());
	
	clang::CodeCompleteOptions ccOpts;
	ccOpts.IncludeMacros = mOptions.codeCompleteIncludeMacros ? 1 : 0;
	ccOpts.IncludeCodePatterns = mOptions.codeCompleteIncludeCodePatterns ? 1 : 0;
	ccOpts.IncludeGlobals = mOptions.codeCompleteIncludeGlobals ? 1 : 0;
	ccOpts.IncludeBriefComments = mOptions.codeCompleteIncludeBriefComments ? 1 : 0;

	Clara::CodeCompleteConsumer consumer(ccOpts);
	consumer.includeOptionalArguments = mOptions.codeCompleteIncludeOptionalArguments;
	LangOptions langOpts = mUnit->getLangOpts();

	mDiags->Reset();
	mStoredDiags.clear();
	IntrusiveRefCntPtr<SourceManager> sourceManager(new SourceManager(*mDiags, *mFileMgr));
	mUnit->CodeComplete(mOptions.filename, row, column, remappedFiles, 
		mOptions.codeCompleteIncludeMacros, mOptions.codeCompleteIncludeCodePatterns, 
		mOptions.codeCompleteIncludeBriefComments, consumer,
		mPchOps, *mDiags, langOpts, 
		*sourceManager, *mFileMgr, mStoredDiags, mOwnedBuffers);
	std::vector<std::pair<std::string, std::string>> results;
	consumer.moveResult(results);
	return results;
}

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	using namespace clang::frontend;
	pybind11::gil_scoped_release releaser;
	return codeCompleteImpl(unsavedBuffer, row, column);
}

void Session::codeCompleteAsync(const int viewID, std::string unsavedBuffer, int row, int column, pybind11::object callback)
{
	using namespace clang;
	using namespace clang::frontend;
	std::thread task( [this, viewID, row, column, callback{std::move(callback)}, unsavedBuffer{std::move(unsavedBuffer)} ] () -> void
	{
		// We want only one thread at a time to execute this
		std::unique_lock<std::mutex> methodLock(mMethodMutex);
		const auto results = codeCompleteImpl(unsavedBuffer.c_str(), row, column);
		methodLock.unlock(); // unlock it, we're done with mUnit
		try
		{
			pybind11::gil_scoped_acquire pythonLock;
			callback(viewID, row, column, results);
		}
		catch (const std::exception& err)
		{
			report(err.what());
		}
	});
	task.detach();
}

void Session::reparse(const int viewID, pybind11::object reparseCallback)
{
	pybind11::gil_scoped_release releaser;
	bool success = !mUnit->Reparse(mPchOps);
	pybind11::gil_scoped_acquire pythonLock;
	if (reparseCallback == pybind11::object()) return;
	else reparseCallback(viewID, success);
}

void Session::save() const
{
	mUnit->Save(mOptions.astFile);
}

const std::string& Session::getFilename() const noexcept
{
	return mOptions.filename;
}

void Session::report(const char* message) const
{
	pybind11::gil_scoped_acquire lock;
	if (message != nullptr && mOptions.logCallback != pybind11::object())
	{
		// yeah this is horrible.
		// but Python has no concept of const,
		// so it somehow works fine.
		const_cast<Session*>(this)->mOptions.logCallback(message);
	}
}

