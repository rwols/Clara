#include "CompilationDatabaseWatcher.hpp"
#include <thread>

namespace Clara
{

std::map<int, std::unique_ptr<clang::tooling::CompilationDatabase>>
    CompilationDatabaseWatcher::mDatabases =
        std::map<int, std::unique_ptr<clang::tooling::CompilationDatabase>>();

std::mutex CompilationDatabaseWatcher::mMethodMutex;

pybind11::module sublime = pybind11::module::import("sublime");

void CompilationDatabaseWatcher::onNew(pybind11::object view)
{
    if (view.is_none()) return;
    const auto filename = view.attr("file_name")();
    if (filename.is_none()) return;
    pybind11::print("clara: checking compdb for", filename);
    auto settings = view.attr("settings")();
    if (settings.is_none())
    {
        pybind11::print("clara:", filename, "has no settings");
        return;
    }
    auto window = view.attr("window")();
    if (window.is_none())
    {
        pybind11::print("clara:", filename, "has no window associated to it");
        return;
    }
    const auto window_id = window.attr("id")().cast<int>();

    // Check if the view has compile commands.
    auto compile_commands =
        settings.attr("get")("compile_commands", pybind11::none());
    if (compile_commands.is_none())
    {
        pybind11::print("clara:", filename,
                        "does not have compile_commands, skipping");
        return;
    }

    // Check if the view has the correct syntax.
    const auto syntax = settings.attr("get")("syntax", "").cast<std::string>();
    if (syntax.find("C++") == std::string::npos)
    {
        pybind11::print("clara:", filename, "does not have the right syntax");
        return;
    }

    // OK, everything looks good at this point. Start enabling the code
    // completer.
    std::unique_lock<std::mutex> lock(mMethodMutex);
    const auto findResult = mDatabases.find(window_id);
    if (findResult != mDatabases.end())
    {
        lock.unlock();
        if (std::get<1>(this->getForView(view)).empty())
        {
            pybind11::print("clara:", filename,
                            "doesn't have compile commands");
            return;
        }
        if (!settings.attr("get")("_clara_code_completer", false).cast<bool>())
        {
            settings.attr("set")("_clara_code_completer", true);
        }
        return;
    }
    pybind11::print("clara: loading compile commands for", filename);
    compile_commands = sublime.attr("expand_variables")(
        compile_commands, window.attr("extract_variables")());
    const auto compilation_dir = compile_commands.cast<std::string>();
    std::string error_message;
    std::unique_ptr<clang::tooling::CompilationDatabase> database =
        clang::tooling::CompilationDatabase::autoDetectFromDirectory(
            compilation_dir, error_message);
    if (!database)
    {
        pybind11::print("clara: failed to load compilation database for ",
                        filename);
        return;
    }
    if (!error_message.empty())
    {
        pybind11::print("clara:", error_message);
        return;
    }
    mDatabases.emplace(window_id, std::move(database));
    lock.unlock();
    pybind11::print("clara: enabling code completer for", filename);
    settings.attr("set")("_clara_code_completer", true);
}

void CompilationDatabaseWatcher::onLoad(pybind11::object view)
{
    onNew(std::move(view));
}

void CompilationDatabaseWatcher::onClone(pybind11::object view)
{
    onNew(std::move(view));
}

void CompilationDatabaseWatcher::onActivated(pybind11::object view)
{
    onNew(std::move(view));
}

std::tuple<std::vector<std::string>, std::string>
CompilationDatabaseWatcher::getForView(pybind11::object view)
{
    std::lock_guard<std::mutex> lock(mMethodMutex);
    std::vector<std::string> result;
    const auto findResult =
        mDatabases.find(view.attr("window")().attr("id")().cast<int>());
    if (findResult == mDatabases.end())
    {
        return std::make_tuple(result, "");
    }
    const auto filename = view.attr("file_name")().cast<std::string>();
    const auto compile_commands =
        findResult->second->getCompileCommands(filename);
    if (compile_commands.empty())
    {
        return std::make_tuple(result, "");
    }
    // FIXME: What to do in case there are multiple (different) compile commands
    // for the same translation unit? Right now we take the first that we
    // encounter. This fails to give correct completion results when there are
    // different #define's on the command line for the different invocations,
    // just to give an example.
    const auto &compile_command = compile_commands[0];
    // Skip first argument, because that's the path to the compiler.
    for (std::size_t i = 1; i < compile_command.CommandLine.size(); ++i)
    {
        const auto &str = compile_command.CommandLine[i];
        if (!str.empty()) result.emplace_back(str.c_str());
    }
    return std::make_tuple(std::move(result), compile_commands[0].Directory);
}

void CompilationDatabaseWatcher::registerClass(pybind11::module &m)
{
    using namespace pybind11;
    class_<CompilationDatabaseWatcher>(m, "CompilationDatabaseWatcher")
        .def(init<>())
        .def("on_new", &CompilationDatabaseWatcher::onNew)
        .def("on_load", &CompilationDatabaseWatcher::onLoad)
        .def("on_clone", &CompilationDatabaseWatcher::onClone)
        .def("on_activated", &CompilationDatabaseWatcher::onActivated)
        .def_static("get_for_view", &CompilationDatabaseWatcher::getForView);
}

} // Clara
