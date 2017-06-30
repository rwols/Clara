#pragma once

#include "PyBind11.hpp"

namespace Clara
{

template <typename... Args>
void claraPrint(const pybind11::object &view, Args &&... args)
{
    if (view.attr("settings")().attr("get")("clara_debug", false).cast<bool>())
    {
        pybind11::print("clara:", std::forward<Args>(args)...);
    }
}

} // Clara
