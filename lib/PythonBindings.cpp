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
        .def_readwrite("diagnosticCallback",
                       &SessionOptions::diagnosticCallback,
                       "The diagnostic callable handling diagnostic callbacks.")
        .def_readwrite(
            "logCallback", &SessionOptions::logCallback,
            "A callable python object that accepts strings as single argument.")
        .def_readwrite(
            "codeCompleteCallback", &SessionOptions::codeCompleteCallback,
            "A callable python object that accepts a list of pairs of strings.")
        .def_readwrite("filename", &SessionOptions::filename,
                       "The filename of the session.")
        .def_readwrite("systemHeaders", &SessionOptions::systemHeaders,
                       "The system headers for the session.")
        .def_readwrite(
            "frameworks", &SessionOptions::frameworks,
            "The list of directories where OSX system frameworks reside.")
        .def_readwrite("builtinHeaders", &SessionOptions::builtinHeaders,
                       "The builtin headers for the session.")
        .def_readwrite("astFile", &SessionOptions::astFile,
                       "The AST file corresponding to this filename.")
        .def_readwrite("invocation", &SessionOptions::invocation,
                       "The command line arguments for this translation unit.")
        .def_readwrite(
            "workingDirectory", &SessionOptions::workingDirectory,
            "The working directory for the command line arguments in "
            "the invocation.")
        .def_readwrite(
            "languageStandard", &SessionOptions::languageStandard,
            "The language standard (lang_cxx11, lang_cxx14, lang_cxx1z)")
        .def_readwrite("codeCompleteIncludeMacros",
                       &SessionOptions::codeCompleteIncludeMacros)
        .def_readwrite("codeCompleteIncludeCodePatterns",
                       &SessionOptions::codeCompleteIncludeCodePatterns)
        .def_readwrite("codeCompleteIncludeGlobals",
                       &SessionOptions::codeCompleteIncludeGlobals)
        .def_readwrite("codeCompleteIncludeBriefComments",
                       &SessionOptions::codeCompleteIncludeBriefComments);

    register_exception<Session::ASTFileReadError>(m, "ASTFileReadError");
    register_exception<Session::ASTParseError>(m, "ASTParseError");

    class_<Session>(m, "Session", "Holds a unique pointer to the \"translation\
 unit\" of the implementation file of the sublime.View object.")
        .def(init<const SessionOptions &>())
        .def("codeComplete", &Session::codeComplete)
        .def("codeCompleteAsync", &Session::codeCompleteAsync)
        .def("reparse", &Session::reparse)
        .def("filename", &Session::getFilename)
        .def("save", &Session::save)
        .def("__repr__", [](const Session &session) {
            std::string result("<Clara.Session object for file \"");
            result.append(session.getFilename()).append("\">");
            return result;
        });

    return m.ptr();
}
