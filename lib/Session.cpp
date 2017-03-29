#include "Session.hpp"
#include "RegionClassifierConsumer.hpp"
#include "RegionClassifierVisitor.hpp"
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/Utils.h> // for clang::createInvocationFromCommandLine
#include <pybind11/stl.h>
#include <sstream>
#include <thread>

#define SKIP_FUNCTION_BODIES
// #define PRINT_HEADER_SEARCH_PATHS

using namespace Clara;

const char *Session::ASTFileReadError::what() const noexcept
{
    return "ASTFileReadError";
}

const char *Session::ASTParseError::what() const noexcept
{
    return "ASTParseError";
}

Session::Session(const SessionOptions &options)
    : mOptions(options), mDiagOpts(new clang::DiagnosticOptions()),
      mDiagConsumer(mOptions.diagnosticCallback),
      mDiags(new clang::DiagnosticsEngine(&mDiagIds, mDiagOpts.get(),
                                          &mDiagConsumer, false))
{
    using namespace clang;

    std::unique_lock<std::mutex> constructLock(mMethodMutex);
    pybind11::gil_scoped_release releaser;

    mFileOpts.WorkingDir = mOptions.workingDirectory;
    mFileMgr = new FileManager(mFileOpts);
    // TODO: Uncomment these lines someday...
    // llvm::sys::fs::file_status astStatus, fileStatus;
    // llvm::sys::fs::status(mOptions.filename, fileStatus);
    // llvm::sys::fs::status(mOptions.astFile, astStatus);
    // if (llvm::sys::fs::exists(astStatus))
    // {
    //  const auto astFileLastModTime = astStatus.getLastModificationTime();
    //  const auto srcFileLastModTime = fileStatus.getLastModificationTime();
    //  if (srcFileLastModTime <= astFileLastModTime)
    //  {
    //    // TODO: For each include file, check wether those includes are
    // also
    //    // up to date.
    //    std::string message("Reading \"");
    //    message.append(mOptions.astFile);
    //    message.append("\".");
    //    report(message.c_str());
    //    mUnit = ASTUnit::LoadFromASTFile(
    //      mOptions.astFile,
    //      mPchOps->getRawReader(),
    //      mDiags,
    //      mFileOpts,
    //      /*UseDebugInfo*/ false,
    //      /*OnlyLocalDecls*/ false,
    //      /*RemappedFiles*/ llvm::None,
    //      /*CaptureDiagnostics*/ false,
    //      /*AllowPCHWithCompilerErrors*/ true,
    //      /*UserFilesAreVolatile*/ true);
    //    if (mUnit)
    //    {
    //      report("Succesfully parsed existing AST file.");
    //      return;
    //    }
    //    else
    //    {
    //      report("Failed to parse existing AST file. Throwing
    // exception...");
    //      throw ASTFileReadError();
    //    }
    //  }
    // }

    auto invocation =
        std::shared_ptr<CompilerInvocation>(createInvocationFromOptions());
    mUnit = ASTUnit::LoadFromCompilerInvocation(
        invocation, mPchOps, mDiags, mFileMgr.get(),
        /*OnlyLocalDecls*/ false,
        /*CaptureDiagnostics*/ false,
        /*PrecompilePreambleAfterNParses*/ 2,
        /*TranslationUnitKind*/ clang::TU_Complete,
        /*CacheCodeCompletionResults*/ true,
        /*IncludeBriefCommentsInCodeCompletion*/
        mOptions.codeCompleteIncludeBriefComments,
        /*UserFilesAreVolatile*/ true);
    if (!mUnit)
    {
        throw ASTParseError();
    }
    if (mUnit->Reparse(mPchOps))
    {
        throw ASTParseError();
    }

    auto workerLambda = [this]() {
        report("reporting for duty");
        std::unique_lock<std::mutex> lock(mMethodMutex);
        while (true)
        {
            report("going to sleep");
            mConditionVar.wait(lock, [this]() { return mViewID != 0; });
            if (mViewID == -1) break;
            report("woke up, starting completion run");

            // reparsing the ASTUnit makes sure that the preamble is up-to-date
            mUnit->Reparse(mPchOps);
            auto results =
                codeCompleteImpl(mUnsavedBuffer.c_str(), mRow, mColumn);
            try
            {
                pybind11::gil_scoped_acquire pythonLock;
                mOptions.codeCompleteCallback(mViewID, mRow, mColumn,
                                              std::move(results));
            }
            catch (const std::exception &err)
            {
                report(err.what());
            }
            mViewID = 0;
        }
        report("goodbye!");
    };

    mViewID = 0;
    constructLock.unlock();
    std::thread(workerLambda).detach();
}

Session::~Session()
{
    // makes the worker thread stop execution
    mViewID = -1;
    mConditionVar.notify_one();
}

clang::CompilerInvocation *Session::createInvocationFromOptions()
{
    using namespace clang;
    std::unique_ptr<CompilerInvocation> invocation;
    if (!mOptions.invocation.empty())
    {
        std::vector<const char *> commandLine;
        for (const auto &str : mOptions.invocation)
            commandLine.push_back(str.c_str());
        invocation = createInvocationFromCommandLine(commandLine, mDiags);
        if (invocation)
        {
            invocation->getFileSystemOpts().WorkingDir =
                mOptions.workingDirectory;
            // mFileOpts.WorkingDir = mOptions.workingDirectory;
            fillInvocationWithStandardHeaderPaths(invocation.get());
            return invocation.release();
        }
    }
    else
    {
        invocation = makeInvocation();
    }
    return invocation.release();
}

std::unique_ptr<clang::CompilerInvocation> Session::makeInvocation() const
{
    using namespace clang;
    auto invocation = std::make_unique<CompilerInvocation>();
    invocation->TargetOpts->Triple = llvm::sys::getDefaultTargetTriple();
    invocation->setLangDefaults(*invocation->getLangOpts(), IK_CXX,
                                llvm::Triple(invocation->TargetOpts->Triple),
                                invocation->getPreprocessorOpts(),
                                mOptions.languageStandard);

    auto &frontendOpts = invocation->getFrontendOpts();
    frontendOpts.Inputs.emplace_back(
        getFilename(),
        FrontendOptions::getInputKindForExtension(getFilename()));

    fillInvocationWithStandardHeaderPaths(invocation.get());

    return invocation;
}

void Session::fillInvocationWithStandardHeaderPaths(
    clang::CompilerInvocation *invocation) const
{
    auto &headerSearchOpts = invocation->getHeaderSearchOpts();

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
    for (const auto &systemHeader : mOptions.systemHeaders)
    {
        addPath(invocation, systemHeader, false);
    }
    for (const auto &framework : mOptions.frameworks)
    {
        addPath(invocation, framework, true);
    }
}

void Session::addPath(clang::CompilerInvocation *invocation,
                      const std::string &path, bool isFramework) const
{
    auto &headerSearchOpts = invocation->getHeaderSearchOpts();

#if PRINT_HEADER_SEARCH_PATHS
    std::string message("Adding system include path \"");
    message.append(path);
    message.append("\".");
    report(message.c_str());
#endif // PRINT_HEADER_SEARCH_PATHS

    headerSearchOpts.AddPath(path, clang::frontend::System, isFramework,
                             /*ignoreSysRoot=*/false);
}

void Session::resetDiagnosticsEngine()
{
    // IntrusiveRefCntPtr takes care of deleting things
    using namespace clang;
    mDiagOpts = new DiagnosticOptions();
    mDiags = new DiagnosticsEngine(&mDiagIds, mDiagOpts.get(), &mDiagConsumer,
                                   false);
    mSourceMgr = new SourceManager(*mDiags, *mFileMgr);
}

std::vector<std::pair<std::string, std::string>>
Session::codeCompleteImpl(const char *unsavedBuffer, int row, int column)
{
    using namespace clang;
    using namespace clang::frontend;
    SmallVector<ASTUnit::RemappedFile, 1> remappedFiles;
    auto memBuffer = llvm::MemoryBuffer::getMemBufferCopy(unsavedBuffer);
    remappedFiles.emplace_back(mOptions.filename, memBuffer.get());

    clang::CodeCompleteOptions ccOpts;
    ccOpts.IncludeMacros = mOptions.codeCompleteIncludeMacros ? 1 : 0;
    ccOpts.IncludeCodePatterns =
        mOptions.codeCompleteIncludeCodePatterns ? 1 : 0;
    ccOpts.IncludeGlobals = mOptions.codeCompleteIncludeGlobals ? 1 : 0;
    ccOpts.IncludeBriefComments =
        mOptions.codeCompleteIncludeBriefComments ? 1 : 0;

    Clara::CodeCompleteConsumer consumer(ccOpts);
    consumer.includeOptionalArguments =
        mOptions.codeCompleteIncludeOptionalArguments;
    LangOptions langOpts = mUnit->getLangOpts();

    IntrusiveRefCntPtr<SourceManager> sourceManager(
        new SourceManager(*mDiags, *mFileMgr));
    mUnit->CodeComplete(mOptions.filename, row, column, remappedFiles,
                        mOptions.codeCompleteIncludeMacros,
                        mOptions.codeCompleteIncludeCodePatterns,
                        mOptions.codeCompleteIncludeBriefComments, consumer,
                        mPchOps, *mDiags, langOpts, *sourceManager, *mFileMgr,
                        mStoredDiags, mOwnedBuffers);
    std::vector<std::pair<std::string, std::string>> results;
    consumer.moveResult(results);
    return results;
}

std::vector<std::pair<std::string, std::string>>
Session::codeComplete(const char *unsavedBuffer, int row, int column)
{
    using namespace clang;
    using namespace clang::frontend;
    pybind11::gil_scoped_release releaser;
    return codeCompleteImpl(unsavedBuffer, row, column);
}

void Session::codeCompleteAsync(const int viewID, std::string unsavedBuffer,
                                int row, int column, pybind11::object callback)
{
    using namespace clang;
    using namespace clang::frontend;

    // No use in code-completion if there is no callback.
    if (mOptions.codeCompleteCallback.is_none()) return;

    std::unique_lock<std::mutex> methodLock(mMethodMutex, std::try_to_lock);
    if (!methodLock)
    {
        report("Failed to acquire lock.");
        return;
    }
    if (mViewID != 0)
    {
        report("ViewID is not zero, aborting.");
        return;
    }
    mViewID = viewID;
    mRow = row;
    mColumn = column;
    mUnsavedBuffer = std::move(unsavedBuffer);
    report("releasing method lock");
    methodLock.unlock();
    report("notifying worker thread");
    mConditionVar.notify_one();
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

const std::string &Session::getFilename() const noexcept
{
    return mOptions.filename;
}

void Session::report(const char *message) const
{
    pybind11::gil_scoped_acquire lock;
    if (mOptions.logCallback.is_none()) return;
    std::ostringstream ss;
    ss << std::this_thread::get_id() << ": " << message;
    const_cast<Session *>(this)->mOptions.logCallback(ss.str());
}

void Session::addRegionForEntity(
    std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
        &map,
    const char *entityname, const clang::Decl *decl) const
{
    std::string msg = "processing decl: ";
    llvm::raw_string_ostream ss(msg);
    decl->dump(ss);
    ss.flush();
    report(msg.c_str());
    // auto start = decl->getLocStart();
    // start = mUnit->mapLocationFromPreamble(start);
    // start = mSourceMgr->getSpellingLoc(start);
    // if (start.isInvalid() || !mUnit->isInMainFileID(start))
    // {
    //     return;
    // }
    // auto end = decl->getLocStart();
    // end = mUnit->mapLocationFromPreamble(end);
    // end = mSourceMgr->getSpellingLoc(end);
    // if (start.isInvalid() || !mUnit->isInMainFileID(start))
    // {
    //     return;
    // }
    // const auto startOffset = mSourceMgr->getFileOffset(start);
    // const auto endOffset = mSourceMgr->getFileOffset(end);
    // map[entityname].emplace_back(startOffset, endOffset);
    // mSourceMgr->getSpellingLoc(mUnit->get)
    // if (decl->getLocStart().isInvalid()) return;
    // if (decl->getLocEnd().isInvalid()) return;
    // const auto start = mSourceMgr->getSpellingLoc(decl->getLocStart());
    // const auto end = mSourceMgr->getSpellingLoc(decl->getLocEnd());
    // // if (start.isInvalid()) return;
    // // if (end.isInvalid()) return;
    // // if (!mUnit->isInMainFileID(start)) return;
    // // if (!mUnit->isInMainFileID(end)) return;
    // const auto startdecomp = mSourceMgr->getDecomposedLoc(start);
    // const auto enddecomp = mSourceMgr->getDecomposedLoc(end);
    // if (startdecomp.first.isInvalid()) return;
    // if (enddecomp.first.isInvalid()) return;
    // const auto startOffset = startdecomp.second;
    // const auto endOffset = enddecomp.second;
    // const auto startOffset = mSourceMgr->getFileOffset(start);
    // const auto endOffset = mSourceMgr->getFileOffset(end);
    // map[entityname].emplace_back(startOffset, endOffset);
}

std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
Session::visitLocalDeclarations()
{
    // std::lock_guard<std::mutex> methodLock(mMethodMutex);
    pybind11::gil_scoped_release releaser;
    // const auto &ctx = mUnit->getASTContext();
    // RegionClassifierConsumer mConsumer;
    // mConsumer.HandleTranslationUnit(const_cast<clang::ASTContext &>(ctx));
    // mConsumer.moveResult(result);
    // return result;

    // std::string msg("processing ");
    // msg.append(std::to_string(mUnit->top_level_size()))
    //     .append(" top level decls");
    // report(msg.c_str());

    RegionClassifierVisitor visitor(mUnit.get(), mOptions.logCallback);
    for (auto decliter = mUnit->top_level_begin();
         decliter != mUnit->top_level_end(); ++decliter)
    {
        const auto decl = *decliter;
        visitor.TraverseDecl(decl);
        //     msg = "";
        //     llvm::raw_string_ostream ss(msg);
        //     decl->dump(ss);
        //     ss.flush();
        //     report(msg.c_str());
        //     if (const auto tag = llvm::dyn_cast<const clang::TagDecl>(decl))
        //     {
        //         // For the scope names, see:
        //         // https://www.sublimetext.com/docs/3/scope_naming.html
        //         if (tag->isNamespace())
        //         {
        //             addRegionForEntity(result, "entity.name.namespace",
        //             decl);
        //         }
        //         else if (tag->isClass())
        //         {
        //             addRegionForEntity(result, "entity.name.class", decl);
        //         }
        //     }
        //     else if (const auto value =
        //                  llvm::dyn_cast<const clang::ValueDecl>(decl))
        //     {
        //         if (const auto t = value->getType().getTypePtrOrNull())
        //         {
        //             if (const auto tag = t->getAsTagDecl())
        //             {
        //                 // For the scope names, see:
        //                 //
        //                 //
        //                 https://www.sublimetext.com/docs/3/scope_naming.html
        //                 if (tag->isNamespace())
        //                 {
        //                     addRegionForEntity(result,
        //                     "entity.name.namespace",
        //                                        decl);
        //                 }
        //                 else if (tag->isClass())
        //                 {
        //                     addRegionForEntity(result, "entity.name.class",
        //                     decl);
        //                 }
        //             }
        //         }
        //     }
    }
    std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
        result;
    visitor.moveResult(result);
    return result;
}
