#include "../include/Session.hpp"
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/Utils.h> // for clang::createInvocationFromCommandLine
#include <pybind11/stl.h>

#define SKIP_FUNCTION_BODIES
// #define PRINT_HEADER_SEARCH_PATHS

using namespace Clara;

const char* Session::ASTFileReadError::what() const noexcept
{
	return "ASTFileReadError";
}

const char* Session::ASTParseError::what() const noexcept
{
	return "ASTParseError";
}

Session::Session(const SessionOptions& options)
: mOptions(options)
, mDiagOpts(new clang::DiagnosticOptions())
, mDiagConsumer(mOptions.diagnosticCallback)
, mDiags(new clang::DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), 
	&mDiagConsumer, false))
{
	using namespace clang;

	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	pybind11::gil_scoped_release releaser;
	
	mFileOpts.WorkingDir = mOptions.workingDirectory;
	mFileMgr = new FileManager(mFileOpts);

	// TODO: Uncomment these lines someday...
	// llvm::sys::fs::file_status astStatus, fileStatus;
	// llvm::sys::fs::status(mOptions.filename, fileStatus);
	// llvm::sys::fs::status(mOptions.astFile, astStatus);
	// if (llvm::sys::fs::exists(astStatus))
	// {
	// 	const auto astFileLastModTime = astStatus.getLastModificationTime();
	// 	const auto srcFileLastModTime = fileStatus.getLastModificationTime();
	// 	if (srcFileLastModTime <= astFileLastModTime)
	// 	{
	// 		// TODO: For each include file, check wether those includes are also
	// 		// up to date.
	// 		std::string message("Reading \"");
	// 		message.append(mOptions.astFile);
	// 		message.append("\".");
	// 		report(message.c_str());
	// 		mUnit = ASTUnit::LoadFromASTFile(
	// 			mOptions.astFile, 
	// 			mPchOps->getRawReader(), 
	// 			mDiags, 
	// 			mFileOpts,
	// 			/*UseDebugInfo*/ false, 
	// 			/*OnlyLocalDecls*/ false,
	// 			/*RemappedFiles*/ llvm::None,
	// 			/*CaptureDiagnostics*/ false,
	// 			/*AllowPCHWithCompilerErrors*/ true,
	// 			/*UserFilesAreVolatile*/ true);
	// 		if (mUnit)
	// 		{
	// 			report("Succesfully parsed existing AST file.");
	// 			return;
	// 		}
	// 		else
	// 		{
	// 			report("Failed to parse existing AST file. Throwing exception...");
	// 			throw ASTFileReadError();
	// 		}
	// 	}
	// }

	IntrusiveRefCntPtr<CompilerInvocation> invocation(createInvocationFromOptions());
	mUnit = ASTUnit::LoadFromCompilerInvocation(
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
	if (!mUnit)
	{
		throw ASTParseError();
	}
	if (mUnit->Reparse(mPchOps))
	{
		throw ASTParseError();
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
	
	#ifdef SKIP_FUNCTION_BODIES
		invocation->getFrontendOpts().SkipFunctionBodies = 1;
	#endif

	#ifdef PRINT_HEADER_SEARCH_PATHS
		headerSearchOpts.Verbose = true;
	#else
		headerSearchOpts.Verbose = false;
	#endif

	headerSearchOpts.UseBuiltinIncludes = false;
	headerSearchOpts.UseStandardSystemIncludes = true;
	headerSearchOpts.UseStandardCXXIncludes = true;
	
	addPath(invocation, mOptions.builtinHeaders, false);
	for (const auto& systemHeader : mOptions.systemHeaders)
	{
		addPath(invocation, systemHeader, false);
	}
	for (const auto& framework : mOptions.frameworks)
	{
		addPath(invocation, framework, true);
	}
}

void Session::addPath(clang::CompilerInvocation* invocation, 
	const std::string& path, bool isFramework) const
{
	auto& headerSearchOpts = invocation->getHeaderSearchOpts();
	std::string message("Adding system include path \"");
	message.append(path);
	message.append("\".");
	report(message.c_str());
	headerSearchOpts.AddPath(path, clang::frontend::System, isFramework, 
		/*ignoreSysRoot=*/ false);
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

	// mDiags = new DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer, false);
	// mDiags->Reset();
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

	// No use in code-completion if there is no callback.
	if (callback.is_none()) return;

	std::thread task( [this, viewID, row, column, callback{std::move(callback)}, unsavedBuffer{std::move(unsavedBuffer)} ] () -> void
	{
		// We want only one thread at a time to execute this
		std::lock_guard<std::mutex> methodLock(mMethodMutex);
		auto results = codeCompleteImpl(unsavedBuffer.c_str(), row, column);
		try
		{
			pybind11::gil_scoped_acquire pythonLock;
			callback(viewID, row, column, std::move(results));
			std::string reportMsg("There are ");
			reportMsg.append(std::to_string(mUnit->stored_diag_size())).append(" diags.");
			report(reportMsg.c_str());
			for (auto iter = mUnit->stored_diag_begin(); iter != mUnit->stored_diag_end(); ++iter)
			{
				report(iter->getMessage().str().c_str());
			}
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
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	bool success;
	{
		pybind11::gil_scoped_release releaser;
		success = !mUnit->Reparse(mPchOps);
	}
	return success;
}

void Session::save() const
{
	std::lock_guard<std::mutex> methodLock(mMethodMutex);
	pybind11::gil_scoped_release releaser;
	mUnit->Save(mOptions.astFile);
}

const std::string& Session::getFilename() const noexcept
{
	return mOptions.filename;
}

void Session::report(const char* message) const
{
	pybind11::gil_scoped_acquire lock;
	if (mOptions.logCallback.is_none()) return;
	const_cast<Session*>(this)->mOptions.logCallback(message);
}
