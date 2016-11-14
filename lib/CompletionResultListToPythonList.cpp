#include "CompletionResultListToPythonList.hpp"
#include <boost/python/list.hpp>

namespace Clara {

PyObject* CompletionResultListToPythonList::convert(const std::vector<std::pair<std::string,std::string>>& from)
{
	boost::python::list to;
	for (const auto& fromPair : from)
	{
		boost::python::list toPair;
		toPair.append(fromPair.first);
		toPair.append(fromPair.second);
		to.append(toPair);
	}

	return boost::python::incref(to.ptr());;
}

} // namespace Clara