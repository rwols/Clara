#include "CompilationDatabase.hpp"
#include "Configuration.hpp"
#include "DiagnosticConsumer.hpp"
#include "Session.hpp"
#include "SessionOptions.hpp"
#include <pybind11/stl.h>

PYBIND11_DECLARE_HOLDER_TYPE(T, llvm::IntrusiveRefCntPtr<T>, true);

PYBIND11_PLUGIN(Clara)
{
    using namespace pybind11;
    using namespace Clara;

    module m("Clara", "Clara plugin");

    m.def("claraVersion", [] { return SUBLIME_VERSION; });
    m.def("claraPlatform", [] { return SUBLIME_PLATFORM; });
    m.def("claraPythonVersion", [] { return PYTHON_VERSION; });
    m.def("claraInitialize", [] {
        static bool notInitialized = true;
        gil_scoped_release releaser;
        if (notInitialized)
        {
            notInitialized = false;
        }
    });

    class_<clang::FileSystemOptions>(m, "FileSystemOptions")
        .def(init<>())
        .def_readwrite("working_dir", &clang::FileSystemOptions::WorkingDir);

    class_<clang::FileManager, llvm::IntrusiveRefCntPtr<clang::FileManager>>(
        m, "FileManager")
        .def(init<const clang::FileSystemOptions &>());

    class_<CompilationDatabase>(m, "CompilationDatabase")
        .def(init<const std::string &>())
        .def("get", &CompilationDatabase::operator[]);

    class_<SessionOptions>(m, "SessionOptions")
        .def(init<>())
        .def_readwrite("file_manager", &SessionOptions::fileManager,
                       "The project-wide opaque FileManager object.")
        .def_readwrite("diagnostic_callback",
                       &SessionOptions::diagnosticCallback,
                       "The diagnostic callable handling diagnostic callbacks.")
        .def_readwrite(
            "log_callback", &SessionOptions::logCallback,
            "A callable python object that accepts strings as single argument.")
        .def_readwrite(
            "code_complete_callback", &SessionOptions::codeCompleteCallback,
            "A callable python object that accepts a list of pairs of strings.")
        .def_readwrite("file_name", &SessionOptions::filename,
                       "The filename of the session.")
        .def_readwrite("system_headers", &SessionOptions::systemHeaders,
                       "The system headers for the session.")
        .def_readwrite(
            "frameworks", &SessionOptions::frameworks,
            "The list of directories where OSX system frameworks reside.")
        .def_readwrite("builtin_headers", &SessionOptions::builtinHeaders,
                       "The builtin headers for the session.")
        .def_readwrite("ast_file", &SessionOptions::astFile,
                       "The AST file corresponding to this filename.")
        .def_readwrite("invocation", &SessionOptions::invocation,
                       "The command line arguments for this translation unit.")

        // obsolete?
        .def_readwrite(
            "working_dir", &SessionOptions::workingDirectory,
            "The working directory for the command line arguments in "
            "the invocation.")

        .def_readwrite(
            "language_standard", &SessionOptions::languageStandard,
            "The language standard (lang_cxx11, lang_cxx14, lang_cxx1z)")
        .def_readwrite("code_complete_include_macros",
                       &SessionOptions::codeCompleteIncludeMacros)
        .def_readwrite("code_complete_include_code_patterns",
                       &SessionOptions::codeCompleteIncludeCodePatterns)
        .def_readwrite("code_complete_include_globals",
                       &SessionOptions::codeCompleteIncludeGlobals)
        .def_readwrite("code_complete_include_brief_comments",
                       &SessionOptions::codeCompleteIncludeBriefComments);

    register_exception<Session::ASTFileReadError>(m, "ASTFileReadError");
    register_exception<Session::ASTParseError>(m, "ASTParseError");

    class_<Session>(m, "Session", "Holds a unique pointer to the \"translation\
 unit\" of the implementation file of the sublime.View object.")
        .def(init<const SessionOptions &>())
        .def("code_complete", &Session::codeComplete)
        .def("code_complete_async", &Session::codeCompleteAsync)
        .def("reparse", &Session::reparse)
        .def("filename", &Session::getFilename)
        .def("save", &Session::save)
        .def("set_code_complete_include_macros",
             &Session::setCodeCompleteIncludeMacros)
        .def("set_code_complete_include_code_patterns",
             &Session::setCodeCompleteIncludeCodePatterns)
        .def("set_code_complete_include_globals",
             &Session::setCodeCompleteIncludeGlobals)
        .def("set_code_complete_include_brief_comments",
             &Session::setCodeCompleteIncludeBriefComments)
        .def("set_code_complete_include_optional_arguments",
             &Session::setCodeCompleteIncludeOptionalArguments)
        .def("__repr__", [](const Session &session) {
            std::string result("<Clara.Session object for file \"");
            result.append(session.getFilename()).append("\">");
            return result;
        });

    return m.ptr();
}
