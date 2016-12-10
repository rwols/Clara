#include "../include/Session.hpp"
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/Utils.h> // for clang::createInvocationFromCommandLine
#include <pybind11/stl.h>

#define DEBUG_PRINT llvm::errs() << __FILE__ << ':' << __LINE__ << '\n'

using namespace Clara;

Session::Session(const SessionOptions& options)
: mOptions(options)
, mFileMgr(new clang::FileManager(mFileOpts))
, mDiagOpts(new clang::DiagnosticOptions())
, mDiagConsumer(mOptions.diagnosticCallback)
, mDiags(new clang::DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer, false))
, mSourceMgr(new clang::SourceManager(*mDiags, *mFileMgr))
{
	assert(mSourceMgr == &mDiags->getSourceManager() && "Expected these to be the same...");
	pybind11::gil_scoped_release releaser;
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	clang::IntrusiveRefCntPtr<clang::CompilerInvocation> invocation(createInvocationFromOptions());
	mUnit = clang::ASTUnit::LoadFromCompilerInvocation(
		invocation.get(), 
		mPchOps, 
		mDiags, 
		mFileMgr.get(),
		/*OnlyLocalDecls*/ false, 
		/*CaptureDiagnostics*/ false,
		/*PrecompilePreambleAfterNParses*/ 1,
		/*TranslationUnitKind*/ clang::TU_Complete, 
		/*CacheCodeCompletionResults*/ true,
		/*IncludeBriefCommentsInCodeCompletion*/ mOptions.codeCompleteIncludeBriefComments, 
		/*UserFilesAreVolatile*/ true);
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
			mFileOpts.WorkingDir = mOptions.workingDirectory;
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

// clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> Session::createDiagnosticsEngine() const
// {
// 	using namespace clang;
// 	IntrusiveRefCntPtr<DiagnosticOptions> diagOpts(new DiagnosticOptions());
// 	IntrusiveRefCntPtr<DiagnosticsEngine> diagEngine()
// 	mDiags(new clang::DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer, false))
// }

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

	Clara::CodeCompleteConsumer consumer(ccOpts/*, mFileMgr, mOptions.filename, row, column*/);
	LangOptions langOpts = mUnit->getLangOpts();
	// IntrusiveRefCntPtr<SourceManager> sourceManager(new SourceManager(*mDiags, *mFileMgr));
	// SmallVector<StoredDiagnostic, 8> diagnostics;
	// SmallVector<const llvm::MemoryBuffer *, 1> temporaryBuffers;
	mUnit->CodeComplete(mOptions.filename, row, column, remappedFiles, 
		mOptions.codeCompleteIncludeMacros, mOptions.codeCompleteIncludeCodePatterns, 
		mOptions.codeCompleteIncludeBriefComments, consumer,
		mPchOps, *mDiags, langOpts, 
		mDiags->getSourceManager(), *mFileMgr, mStoredDiags, mOwnedBuffers);
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

void Session::codeCompleteAsync(std::string unsavedBuffer, int row, int column, pybind11::object callback)
{
	using namespace clang;
	using namespace clang::frontend;
	std::thread task( [this, row, column, callback{std::move(callback)}, unsavedBuffer{std::move(unsavedBuffer)} ] () -> void
	{
		// We want only one thread at a time to execute this
		std::unique_lock<std::mutex> methodLock(mMethodMutex);
		const auto results = codeCompleteImpl(unsavedBuffer.c_str(), row, column);
		methodLock.unlock(); // unlock it, we're done with mUnit
		try
		{
			pybind11::gil_scoped_acquire pythonLock;
			callback(mOptions.filename, row, column, results);
		}
		catch (const std::exception& err)
		{
			report(err.what());
		}
	});
	task.detach();
}

bool Session::reparse()
{
	// pybind11::gil_scoped_release releaser;
	return !mUnit->Reparse(mPchOps);
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

