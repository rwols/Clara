#include "CodeCompleter.hpp"
#include "CompilationDatabaseWatcher.hpp"
#include "Configuration.hpp"
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace pybind11
{
namespace detail
{
template <>
class type_caster<std::vector<std::pair<std::string, std::string>>>
    : public type_caster_base<std::vector<std::pair<std::string, std::string>>>
{
};
template <>
class type_caster<std::vector<std::string>>
    : public type_caster_base<std::vector<std::string>>
{
};
} // namespace detail
} // namespace pybind11

PYBIND11_PLUGIN(Clara)
{
    using namespace pybind11;
    using namespace Clara;
    module m("Clara", "Clara plugin");
    m.def("version", [] { return SUBLIME_VERSION; });
    CompilationDatabaseWatcher::registerClass(m);
    CodeCompleter::registerClass(m);
    return m.ptr();
}
