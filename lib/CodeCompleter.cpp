#include "CodeCompleter.hpp"
#include "CompilationDatabaseWatcher.hpp"
#include "claraPrint.hpp"
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/Utils.h> // for clang::createInvocationFromCommandLine
#include <future>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <sstream>
#include <thread>

static std::string getHeadersKey()
{
    auto getpass = pybind11::module::import("getpass");
    auto socket = pybind11::module::import("socket");
    auto username = getpass.attr("getuser")().cast<std::string>();
    auto hostname = socket.attr("gethostname")().cast<std::string>();
    return username + "@" + hostname;
}

static std::shared_ptr<clang::GlobalCodeCompletionAllocator>
    gCodeCompleteAlloc(new clang::GlobalCodeCompletionAllocator());

namespace Clara
{

clang::CodeCompleteOptions CodeCompleter::initCodeCompleteOptions() const
{
    clang::CodeCompleteOptions options;
    pybind11::module sublime = pybind11::module::import("sublime");
    auto settings = sublime.attr("load_settings")("Clara.sublime-settings");
    auto get = settings.attr("get");
    options.IncludeMacros = get("code_complete_include_macros", true) ? 1 : 0;
    options.IncludeCodePatterns =
        get("code_complete_code_patterns", true) ? 1 : 0;
    options.IncludeGlobals = get("code_complete_include_globals", true) ? 1 : 0;
    options.IncludeBriefComments =
        get("code_complete_include_brief_comments", false) ? 1 : 0;
    return options;
}

CodeCompleter::CodeCompleter(pybind11::object view)
    : clang::DiagnosticConsumer{},
      clang::CodeCompleteConsumer{initCodeCompleteOptions(), false},
      mCCTUInfo{gCodeCompleteAlloc}, mView{std::move(view)},
      mDiagIds{new clang::DiagnosticIDs()},
      mDiagOpts{new clang::DiagnosticOptions()},
      mDiags{new clang::DiagnosticsEngine{mDiagIds.get(), mDiagOpts.get(), this,
                                          false}}
{
    pybind11::module sublime = pybind11::module::import("sublime");
    claraPrint(mView, "constructing CodeCompleter");
    mFilename = mView.attr("file_name")().cast<std::string>();
    const auto compileCommand = CompilationDatabaseWatcher::getForView(mView);
    auto command = std::get<0>(compileCommand);
    mFileOpts.WorkingDir = std::get<1>(compileCommand);
    if (mFileOpts.WorkingDir.empty() || command.empty())
    {
        throw std::runtime_error("no compile command");
    }
    auto settings = sublime.attr("load_settings")("Clara.sublime-settings");
    auto getsetting = settings.attr("get");
    const auto headersKey = getHeadersKey();
    claraPrint(mView, "loading key", headersKey);
    auto headersDict = getsetting(headersKey, pybind11::none());
    if (headersDict.is_none())
    {
        sublime.attr("error_message")(
            "Headers have not been set up (attempted to access " + headersKey +
            "). Please set up headers and restart sublime.");
        return;
    }
    claraPrint(mView, "loading system headers");
    auto systemHeaders =
        headersDict["system_headers"].cast<std::vector<std::string>>();
    claraPrint(mView, "loading system frameworks");
    auto systemFrameworks =
        headersDict["system_frameworks"].cast<std::vector<std::string>>();
    llvm::SmallString<64> builtinHeadersTemp =
        llvm::StringRef(sublime.attr("packages_path")().cast<std::string>());
    llvm::sys::path::append(builtinHeadersTemp, "Clara", "include");
    std::string builtinHeaders = builtinHeadersTemp.c_str();
    claraPrint(mView, "begin parsing main file");
    mView.attr("set_status")("clara", "parsing...");
    mInitThread =
        std::thread{&CodeCompleter::initAST,     this,
                    std::move(command),          std::move(systemHeaders),
                    std::move(systemFrameworks), std::move(builtinHeaders)};
}

void CodeCompleter::initAST(std::vector<std::string> command,
                            std::vector<std::string> systemHeaders,
                            std::vector<std::string> systemFrameworks,
                            std::string builtinHeaders)
{
    mFileMgr = new clang::FileManager(mFileOpts);
    mSourceMgr = new clang::SourceManager(*mDiags, *mFileMgr);
    std::vector<const char *> commandLine;
    bool skipNext = true;
    for (const auto &str : command)
        if (skipNext)
        {
            skipNext = false;
        }
        else if (str == "-include")
        {
            skipNext = true;
        }
        else
        {
            commandLine.push_back(str.c_str());
        }
    auto invocation =
        clang::createInvocationFromCommandLine(commandLine, mDiags);
    if (invocation)
    {
        invocation->getFileSystemOpts().WorkingDir =
            mFileMgr->getFileSystemOpts().WorkingDir;
        auto &headerSearchOpts = invocation->getHeaderSearchOpts();
        invocation->getFrontendOpts().SkipFunctionBodies = 1;
        // headerSearchOpts.Verbose = true;
        headerSearchOpts.UseBuiltinIncludes = false;
        headerSearchOpts.UseStandardSystemIncludes = true;
        headerSearchOpts.UseStandardCXXIncludes = true;

        addPath(invocation.get(), builtinHeaders, false);
        for (const auto &systemHeader : systemHeaders)
        {
            addPath(invocation.get(), systemHeader, false);
        }
        for (const auto &framework : systemFrameworks)
        {
            addPath(invocation.get(), framework, true);
        }
    }

    mUnit = clang::ASTUnit::LoadFromCompilerInvocation(
        std::shared_ptr<clang::CompilerInvocation>(invocation.release()),
        mPchOps, mDiags, mFileMgr.get(),
        /*OnlyLocalDecls*/ false,
        /*CaptureDiagnostics*/ false,
        /*PrecompilePreambleAfterNParses*/ 2, /* bug, can't set to 1 */
        /*TranslationUnitKind*/ clang::TU_Complete,
        /*CacheCodeCompletionResults*/ true,
        /*IncludeBriefCommentsInCodeCompletion*/ false,
        /*UserFilesAreVolatile*/ true);
    if (!mUnit)
    {
        return;
    }
    if (mUnit->Reparse(mPchOps))
    {
        return;
    }
    mWorkerThread = std::thread(&CodeCompleter::backgroundWorker, this);
    mIsLoaded = true;
    {
        pybind11::gil_scoped_acquire lock;
        claraPrint(mView, "loaded", mFilename);
        mView.attr("erase_status")("clara");
    }
}

void CodeCompleter::backgroundWorker()
{
    std::unique_lock<std::mutex> lock(mMethodMutex);
    while (true)
    {
        mConditionVar.wait(lock, [this]() { return mDoCompletionJob; });
        if (!mIsLoaded) break;

        // reparsing the ASTUnit makes sure that the preamble is
        // up-to-date
        this->reparse();
        codeCompleteImpl();
        {
            pybind11::gil_scoped_acquire pythonLock;
            auto runCommand = mView.attr("run_command");
            runCommand("hide_auto_complete");
            using namespace pybind11::literals; // for the _a literal
            runCommand("auto_complete",
                       "args"_a = pybind11::dict(
                           "disable_auto_insert"_a = true,
                           "api_completions_only"_a = false,
                           "next_completion_if_showing"_a = false));
        }
        mDoCompletionJob = false;
    }
}

void CodeCompleter::codeCompleteImpl()
{
    using namespace clang;
    using namespace clang::frontend;
    SmallVector<ASTUnit::RemappedFile, 1> remappedFiles;
    auto memBuffer =
        llvm::MemoryBuffer::getMemBufferCopy(mUnsavedBuffer.c_str());
    remappedFiles.emplace_back(mFilename, memBuffer.get());
    LangOptions langOpts = mUnit->getLangOpts();
    mDiags->Reset();
    // IntrusiveRefCntPtr<SourceManager> sourceManager(
    //     new SourceManager(*mDiags, *mFileMgr));
    mUnit->CodeComplete(mFilename, mRow, mColumn, remappedFiles,
                        includeMacros(), includeCodePatterns(),
                        /*includeBriefComments()*/ false, *this, mPchOps,
                        *mDiags, langOpts, *mSourceMgr, *mFileMgr, mStoredDiags,
                        mOwnedBuffers);
}

void CodeCompleter::addPath(clang::CompilerInvocation *invocation,
                            const std::string &path, bool isFramework) const
{
    auto &headerSearchOpts = invocation->getHeaderSearchOpts();
    headerSearchOpts.AddPath(path, clang::frontend::System, isFramework,
                             /*ignoreSysRoot=*/false);
}

std::vector<std::pair<std::string, std::string>>
CodeCompleter::onQueryCompletions(pybind11::str prefix,
                                  pybind11::list locations)
{
    claraPrint(mView, "start on_query_completions");
    const auto point = locations[0].cast<unsigned>();
    std::vector<std::pair<std::string, std::string>> empty;
    if (mPoint == point || mPoint + 3 == point)
    {
        mPoint = (unsigned)-1;
        if (!mCompletions.empty())
        {
            claraPrint(mView, "returning completions");
            return std::move(mCompletions);
        }
        else
        {
            claraPrint(mView, "found no completions");
            return empty;
        }
    }
    if (!mIsLoaded)
    {
        claraPrint(mView, "TU is not yet loaded");
        return empty;
    }
    if (pybind11::len(locations) != 1)
    {
        claraPrint(mView, "too many locations");
        return empty;
    }
    pybind11::module sublime = pybind11::module::import("sublime");
    std::tie(mRow, mColumn) = mView.attr("rowcol")(locations[0])
                                  .cast<std::pair<unsigned, unsigned>>();
    mPoint = point;
    mRow++;
    mColumn++;
    auto everything = sublime.attr("Region")(0, mView.attr("size")());
    mUnsavedBuffer = mView.attr("substr")(everything).cast<std::string>();
    claraPrint(mView, "starting code completion run at row", mRow, "column",
               mColumn);
    mDoCompletionJob = true;
    mConditionVar.notify_one();
    return empty;
}

clang::CodeCompletionAllocator &CodeCompleter::getAllocator()
{
    return mCCTUInfo.getAllocator();
}

clang::CodeCompletionTUInfo &CodeCompleter::getCodeCompletionTUInfo()
{
    return mCCTUInfo;
}

void CodeCompleter::registerClass(pybind11::module &m)
{
    using namespace pybind11;
    class_<CodeCompleter>(m, "CodeCompleter")
        .def(init<pybind11::object>())
        .def("on_query_completions", &CodeCompleter::onQueryCompletions)
        .def("on_post_save", &CodeCompleter::onPostSave);
}

void CodeCompleter::ProcessCodeCompleteResults(
    clang::Sema &sema, clang::CodeCompletionContext context,
    clang::CodeCompletionResult *results, unsigned numResults)
{
    // Clear the list, from other runs.
    mCompletions.clear();
    mCompletions.reserve(numResults);

    std::sort(results, results + numResults,
              [](const auto &lhs, const auto &rhs) {
                  return lhs.Priority < rhs.Priority;
              });

    for (unsigned i = 0; i < numResults; ++i)
    {
        if (results[i].Availability == CXAvailability_NotAvailable ||
            results[i].Availability == CXAvailability_NotAccessible)
        {
            continue;
        }
        else
        {
            mCompletions.emplace_back(
                ProcessCodeCompleteResult(sema, context, results[i]));
        }
    }
}

void CodeCompleter::ProcessOverloadCandidates(
    clang::Sema &sema, unsigned currentArg,
    clang::CodeCompleteConsumer::OverloadCandidate *candidates,
    unsigned numCandidates)
{
    for (unsigned i = 0; i < numCandidates; ++i)
    {
        if (auto ccs = candidates[i].CreateSignatureString(
                currentArg, sema, getAllocator(), mCCTUInfo, true))
        {
            unsigned argCount = 0;
            std::string first, second, informative;
            ProcessCodeCompleteString(*ccs, argCount, first, second,
                                      informative);
            if (argCount > 0) second += "$0";
            if (!informative.empty())
            {
                first += "\t";
                first += informative;
            }
            mCompletions.emplace_back(std::move(first), std::move(second));
        }
    }
}

std::pair<std::string, std::string>
CodeCompleter::ProcessCodeCompleteResult(clang::Sema &sema,
                                         clang::CodeCompletionContext context,
                                         clang::CodeCompletionResult &result)
{
    using namespace clang;

    std::string first, second, informative;
    unsigned argCount = 0;

    switch (result.Kind)
    {
    case CodeCompletionResult::RK_Declaration:
    {
        auto completion = result.CreateCodeCompletionString(
            sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
        if (completion == nullptr)
        {
            second = result.Declaration->getNameAsString();
            first = second + "\t[DECL]";
        }
        else
        {
            ProcessCodeCompleteString(*completion, argCount, first, second,
                                      informative);
            if (informative.empty()) informative = "[DECL]";
        }
        break;
    }

    case CodeCompletionResult::RK_Keyword:
        second = result.Keyword;
        first = second + "\t[KEYWORD]";
        break;

    case CodeCompletionResult::RK_Macro:
    {
        auto completion = result.CreateCodeCompletionString(
            sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
        if (completion == nullptr)
        {
            second = result.Macro->getNameStart();
            first = second + "\t[MACRO]";
        }
        else
        {
            ProcessCodeCompleteString(*completion, argCount, first, second,
                                      informative);
            if (informative.empty()) informative = "[MACRO]";
        }
        break;
    }

    case CodeCompletionResult::RK_Pattern:
    {
        auto completion = result.CreateCodeCompletionString(
            sema, context, getAllocator(), mCCTUInfo, includeBriefComments());
        if (completion == nullptr)
        {
            second = result.Macro->getNameStart();
            first = second + "\t[PATTERN]";
        }
        else
        {
            ProcessCodeCompleteString(*completion, argCount, first, second,
                                      informative);
            // FIXME: For some reason, clang reports macro's as patterns ?!
            // Let's not confuse the user and just not put this informational
            // banner in the completion widget.
            // if (informative.empty()) informative = "[PATTERN]";
        }
        break;
    }
    }

    if (argCount > 0)
    {
        second += "$0";
    }

    if (!informative.empty())
    {
        first += "\t";
        first += informative;
    }

    return std::make_pair<std::string, std::string>(std::move(first),
                                                    std::move(second));
}

void CodeCompleter::ProcessCodeCompleteString(
    const clang::CodeCompletionString &ccs, unsigned &argCount,
    std::string &first, std::string &second, std::string &informative) const
{
    using namespace clang;

    for (unsigned j = 0; j < ccs.getAnnotationCount(); ++j)
    {
        const char *annotation = ccs.getAnnotation(j);
        informative += annotation;
        if (j != ccs.getAnnotationCount() - 1) informative += ' ';
    }

    std::string resultType;
    for (const auto &chunk : ccs)
    {
        switch (chunk.Kind)
        {
        case CodeCompletionString::CK_TypedText:
            // The piece of text that the user is expected to type to match the
            // code-completion string,
            // typically a keyword or the name of a declarator or macro.
            first += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_Text:
            // A piece of text that should be placed in the buffer,
            // e.g., parentheses or a comma in a function call.
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_Optional:
            // A code completion string that is entirely optional.
            // For example, an optional code completion string that
            // describes the default arguments in a function call.
            // if (includeOptionalArguments)
            // {
            //     ProcessCodeCompleteString(*chunk.Optional, argCount, first,
            //                               second, informative);
            // }
            break;
        case CodeCompletionString::CK_Placeholder:
            // A string that acts as a placeholder for, e.g., a function call
            // argument.
            ++argCount;
            second += "${";
            second += std::to_string(argCount);
            second += ":";
            // Try to ignore leading underscores for standard library function
            // arguments. This makes the completions cleaner.
            if (strncmp("__", chunk.Text, 2) == 0)
            {
                informative += (chunk.Text + 2);
                second += (chunk.Text + 2);
            }
            else
            {
                informative += chunk.Text;
                second += chunk.Text;
            }
            second += "}";
            break;
        case CodeCompletionString::CK_Informative:
            // A piece of text that describes something about the result
            // but should not be inserted into the buffer.
            informative += chunk.Text;
            break;
        case CodeCompletionString::CK_ResultType:
            // A piece of text that describes the type of an entity or,
            // for functions and methods, the return type.
            resultType = chunk.Text;
            // informative += chunk.Text;
            // informative += " -> ";
            break;
        case CodeCompletionString::CK_CurrentParameter:
            // A piece of text that describes the parameter that corresponds to
            // the code-completion location within a function call, message
            // send,
            // macro invocation, etc.
            ++argCount;
            second += "${";
            second += std::to_string(argCount);
            second += ":";
            // Try to ignore leading underscores for standard library function
            // arguments. This makes the completions cleaner.
            if (strncmp("__", chunk.Text, 2) == 0)
            {
                informative += (chunk.Text + 2);
                second += (chunk.Text + 2);
            }
            else
            {
                informative += chunk.Text;
                second += chunk.Text;
            }
            second += "}";
            break;
        case CodeCompletionString::CK_LeftParen:
            // A left parenthesis ('(').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_RightParen:
            // A right parenthesis (')').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_LeftBracket:
            // A left bracket ('[').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_RightBracket:
            // A right bracket (']').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_LeftBrace:
            // A left brace ('{').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_RightBrace:
            // A right brace ('}').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_LeftAngle:
            // A left angle bracket ('<').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_RightAngle:
            // A right angle bracket ('>').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_Comma:
            // A comma separator (',').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_Colon:
            // A colon (':').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_SemiColon:
            // A semicolon (';').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_Equal:
            // An '=' sign.
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_HorizontalSpace:
            // Horizontal whitespace (' ').
            informative += chunk.Text;
            second += chunk.Text;
            break;
        case CodeCompletionString::CK_VerticalSpace:
            // Vertical whitespace ('\n' or '\r\n', depending on the platform).
            informative += chunk.Text;
            second += chunk.Text;
            break;
        }
    }
    if (first.empty() == false && first.find('~') != std::string::npos)
    {
        informative = "[DESTR]";
    }
    else if (resultType.empty() == false)
    {
        if (informative == "()")
        {
            informative = "(void) -> ";
        }
        else if (informative.find('(') != std::string::npos &&
                 informative.find(')') != std::string::npos)
        {
            informative += " -> ";
        }
        informative += resultType;
    }
    if (ccs.getBriefComment() != nullptr)
    {
        informative += " : ";
        informative += ccs.getBriefComment();
    }
}

void CodeCompleter::onPostSave()
{
    claraPrint(mView, "reparsing...");
    if (mInitThread.joinable()) mInitThread.join();
    mIsLoaded.store(false);
    mInitThread = std::thread{&CodeCompleter::reparse, this};
}

void CodeCompleter::reparse()
{
    // do this twice because we want the preamble to be up to date too. The
    // preamble is reparsed after two calls to ASTUnit::Reparse.
    mUnit->Reparse(mPchOps);
    mUnit->Reparse(mPchOps);
    mIsLoaded.store(true);
    {
        pybind11::gil_scoped_acquire acquire;
        claraPrint(mView, "done reparsing");
    }
}

void CodeCompleter::HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                                     const clang::Diagnostic &info)
{
    clang::DiagnosticConsumer::HandleDiagnostic(level, info);
    if (!mSourceMgr->isInMainFile(info.getLocation()))
    {
        // not interested in diagnostics that are somewhere outside of the
        // file that we're looking at.
        return;
    }
    const auto loc = mSourceMgr->getPresumedLoc(info.getLocation());
    std::ostringstream ss;
    if (loc.isValid())
    {
        ss << loc.getFilename() << ":" << loc.getLine() << ":"
           << loc.getColumn() << ": ";
    }
    else
    {
        ss << "<unknown>: ";
    }
    llvm::SmallString<128> message;
    info.FormatDiagnostic(message);
    ss << message.c_str();
    pybind11::gil_scoped_acquire acquire;
    pybind11::print(ss.str());
}

void CodeCompleter::BeginSourceFile(const clang::LangOptions &options,
                                    const clang::Preprocessor *pp)
{
    clang::DiagnosticConsumer::BeginSourceFile(options, pp);
    pybind11::gil_scoped_acquire acquire;
    claraPrint(mView, "--- BEGIN DIAGNOSTICS ---");
}
void CodeCompleter::EndSourceFile()
{
    clang::DiagnosticConsumer::EndSourceFile();
    pybind11::gil_scoped_acquire acquire;
    claraPrint(mView, "--- END DIAGNOSTICS ---");
}
void CodeCompleter::finish()
{
    clang::DiagnosticConsumer::finish();
    pybind11::gil_scoped_acquire acquire;
    claraPrint(mView, "finished diagnostics");
}

CodeCompleter::~CodeCompleter()
{
    pybind11::gil_scoped_release releaser;
    auto self = mDiags->takeClient();
    if (self.get() == this)
    {
        self.release(); // Don't want to delete ourselves twice!
    }
    if (mInitThread.joinable())
    {
        // Joining the init and worker thread can be user-unfriendly. But I
        // do not know of another way to safely discard these resources.
        mInitThread.join();
    }
    mIsLoaded = false;
    mDoCompletionJob = true;
    mConditionVar.notify_one();
    if (mWorkerThread.joinable())
    {
        mWorkerThread.join();
    }
}

} // Clara
