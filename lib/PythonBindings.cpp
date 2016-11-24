#include <boost/python.hpp>
#include "Session.hpp"
#include "DiagnosticConsumer.hpp"
#include "Configuration.hpp"
#include "SessionOptions.hpp"
#include <clang/Tooling/CompilationDatabase.h>
#include <cassert>

struct CompletionResultListToPythonList
{
	static PyObject* convert(const std::vector<std::pair<std::string,std::string>>& from)
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
};


struct PythonListToStdVectorOfStringsConverter
{
	PythonListToStdVectorOfStringsConverter()
	{
		using namespace boost::python;
		converter::registry::push_back(&convertible, &construct, type_id<std::vector<std::string>>());
	}

	// Determine if obj can be converted in a QString
	static void* convertible(PyObject* obj)
	{
		if (!PySequence_Check(obj) || !PyObject_HasAttrString(obj,"__len__")) return nullptr;
		else return obj;
	}

	// Convert obj into an std::vector<std::string>
	static void construct(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		using namespace boost::python;
		using StringVec = std::vector<std::string>;
		void* storage=((converter::rvalue_from_python_storage<StringVec>*)(data))->storage.bytes;
		new (storage) StringVec();
		auto v = (StringVec*)storage;
		int l = PySequence_Size(obj);
		v->reserve(l);
		for( int i = 0; i < l; ++i)
		{
			v->emplace_back(extract<std::string>(PySequence_GetItem(obj, i)));
		}
		data->convertible = storage;
	}
};

BOOST_PYTHON_MODULE(Clara)
{
	using namespace boost::python;
	using namespace Clara;

	PyEval_InitThreads();

	class_<DiagnosticConsumer>("DiagnosticConsumer")
		.def("beginSourceFile",  &DiagnosticConsumer::beginSourceFile)
		.def("endSourceFile",    &DiagnosticConsumer::EndSourceFile)
		.def("finish",           &DiagnosticConsumer::finish)
		.def("handleDiagnostic", &DiagnosticConsumer::handleDiagnostic)
	;

	class_<SessionOptions>("SessionOptions")
		.def_readwrite("logCallback",                      &SessionOptions::logCallback, "A callable python object that accepts strings as single argument.")
		.def_readwrite("codeCompleteCallback",             &SessionOptions::codeCompleteCallback, "A callable python object that accepts a list of pairs of strings.")
		.def_readwrite("filename",                         &SessionOptions::filename, "The filename of the session.")
		.def_readwrite("systemHeaders",                    &SessionOptions::systemHeaders, "The system headers for the session.")
		.def_readwrite("builtinHeaders",                   &SessionOptions::builtinHeaders, "The builtin headers for the session.")
		.def_readwrite("jsonCompileCommands",              &SessionOptions::jsonCompileCommands, "The directory where the compile commands file resides (in JSON)")
		.def_readwrite("languageStandard",                 &SessionOptions::languageStandard, "The language standard (lang_cxx11, lang_cxx14, lang_cxx1z)")
		.def_readwrite("codeCompleteIncludeMacros",        &SessionOptions::codeCompleteIncludeMacros)
		.def_readwrite("codeCompleteIncludeCodePatterns",  &SessionOptions::codeCompleteIncludeCodePatterns)
		.def_readwrite("codeCompleteIncludeGlobals",       &SessionOptions::codeCompleteIncludeGlobals)
		.def_readwrite("codeCompleteIncludeBriefComments", &SessionOptions::codeCompleteIncludeBriefComments)
	;

	class_<Session, boost::noncopyable>("Session", init<DiagnosticConsumer&, const std::string&>())

		// Constructors
		.def(init<DiagnosticConsumer&, const std::string&, const std::string&>())
		.def(init<const std::string&>())
		.def(init<const std::string&, const std::string&>())
		.def(init<const SessionOptions&>())

		.def_readwrite("reporter",    &Session::reporter)
		.def("codeComplete",          &Session::codeComplete)
		.def("codeCompleteAsync",     &Session::codeCompleteAsync)
		.def("cancelAsyncCompletion", &Session::cancelAsyncCompletion)
		.def("filename",              &Session::getFilename, return_value_policy<copy_const_reference>())
	;

	// Converters

	PythonListToStdVectorOfStringsConverter();
	to_python_converter<std::vector<std::pair<std::string, std::string>>, CompletionResultListToPythonList>();
}