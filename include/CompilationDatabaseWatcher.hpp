#pragma once

#include "PyBind11.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Clara
{

class CompilationDatabaseWatcher
{
  public:
    void onNew(pybind11::object view);
    void onLoad(pybind11::object view);
    void onClone(pybind11::object view);
    void onActivated(pybind11::object view);

    std::tuple<std::vector<std::string>, std::string> static getForView(
        pybind11::object view);

    static void registerClass(pybind11::module &m);

  private:
    static std::mutex mMethodMutex;
    static std::map<int, std::unique_ptr<clang::tooling::CompilationDatabase>>
        mDatabases;
};

} // Clara
