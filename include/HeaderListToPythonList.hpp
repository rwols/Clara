#pragma once

#include <vector>
#include <tuple>
#include <string>

#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif

namespace Clara {

struct HeaderListToPythonList
{
	static PyObject* convert(const std::vector<std::string>& from);
};

} // namespace Clara