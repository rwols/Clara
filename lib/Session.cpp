#include "Session.hpp"
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
{
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	mDiags->setClient(&mDiagConsumer, false);
	pybind11::gil_scoped_release releaser;

	clang::IntrusiveRefCntPtr<clang::CompilerInvocation> invocation(createInvocationFromOptions());
	mUnit = clang::ASTUnit::LoadFromCompilerInvocation(
		invocation.get(), 
		mPchOps, 
		mDiags, 
		mFileMgr.get(),
		/*OnlyLocalDecls*/ false, 
		/*CaptureDiagnostics*/ false, 
		/*PrecompilePreambleAfterNParses*/ 0,
		/*TranslationUnitKind*/ clang::TU_Complete, 
		/*CacheCodeCompletionResults*/ false,
		/*IncludeBriefCommentsInCodeCompletion*/ mOptions.codeCompleteIncludeBriefComments, 
		/*UserFilesAreVolatile*/ false);

	const auto& preamble = mUnit->getPreambleData();
	std::string message("Number of lines in the preamble of \"");
	message.append(mOptions.filename);
	message.append("\" is ");
	message.append(std::to_string(preamble.getNumLines()));
	report(message.c_str());
}

clang::CompilerInvocation* Session::createInvocationFromOptions()
{
	using namespace clang;
	if (!mOptions.invocation.empty())
	{
		std::vector<const char*> commandLine;
		for (const auto& str : mOptions.invocation) commandLine.push_back(str.c_str());
		auto invocation = createInvocationFromCommandLine(commandLine, mDiags);
		if (invocation)
		{
			invocation->getFileSystemOpts().WorkingDir = mOptions.workingDirectory;
			mFileOpts.WorkingDir = mOptions.workingDirectory;
			fillInvocationWithStandardHeaderPaths(invocation);
			return invocation;
		}
	}
	report("Compiler invocation failed.");
	return makeInvocation();
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

std::vector<std::pair<std::string, std::string>> Session::codeComplete(const char* unsavedBuffer, int row, int column)
{
	using namespace clang;
	using namespace clang::frontend;
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	pybind11::gil_scoped_release releaser;
	SmallVector<ASTUnit::RemappedFile, 1> remappedFiles;
	llvm::MemoryBufferRef bufferAsRef(unsavedBuffer, mOptions.filename);
	auto memBuffer = llvm::MemoryBuffer::getMemBuffer(bufferAsRef);
	remappedFiles.push_back({mOptions.filename, memBuffer.release()});

	clang::CodeCompleteOptions mCcOpts;
	mCcOpts.IncludeMacros = mOptions.codeCompleteIncludeMacros ? 1 : 0;
	mCcOpts.IncludeCodePatterns = mOptions.codeCompleteIncludeCodePatterns ? 1 : 0;
	mCcOpts.IncludeGlobals = mOptions.codeCompleteIncludeGlobals ? 1 : 0;
	mCcOpts.IncludeBriefComments = mOptions.codeCompleteIncludeBriefComments ? 1 : 0;
	Clara::CodeCompleteConsumer consumer(mCcOpts, mFileMgr, mOptions.filename, row, column);
	consumer.Diag->setClient(&mDiagConsumer, false);
	consumer.LangOpts = mUnit->getLangOpts();

	mUnit->CodeComplete(mOptions.filename, row, column, remappedFiles, 
		mOptions.codeCompleteIncludeMacros, mOptions.codeCompleteIncludeCodePatterns, 
		mOptions.codeCompleteIncludeBriefComments, consumer,
		mPchOps, *consumer.Diag, consumer.LangOpts, 
		*consumer.SourceMgr, *consumer.FileMgr, consumer.Diagnostics, consumer.TemporaryBuffers);

	std::vector<std::pair<std::string, std::string>> result;
	mCodeCompleteConsumer->moveResult(result);
	return result;
}

void Session::codeCompleteAsync(std::string unsavedBuffer, int row, int column, pybind11::object callback)
{
	using namespace clang;
	using namespace clang::frontend;
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
    std::thread task( [this, row, column, callback{std::move(callback)}, unsavedBuffer{std::move(unsavedBuffer)} ] () -> void
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
        
        Clara::CodeCompleteConsumer consumer(ccOpts, mFileMgr, mOptions.filename, row, column);
        consumer.Diag->setClient(&mDiagConsumer, false);
		mUnit->CodeComplete(mOptions.filename, row, column, remappedFiles, 
			mOptions.codeCompleteIncludeMacros, mOptions.codeCompleteIncludeCodePatterns, 
			mOptions.codeCompleteIncludeBriefComments, consumer,
			mPchOps, *consumer.Diag, consumer.LangOpts, 
			*consumer.SourceMgr, *consumer.FileMgr, consumer.Diagnostics, consumer.TemporaryBuffers);
        std::vector<std::pair<std::string, std::string>> results;
        consumer.moveResult(results);
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

