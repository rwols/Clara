#pragma once

#include <vector>
#include <tuple>
#include <string>

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace Clara {

struct CompletionResultListToPythonList
{
	static PyObject* convert(const std::vector<std::pair<std::string,std::string>>& from);
};

} // namespace Clara