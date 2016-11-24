#include <boost/python.hpp>
#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include "RenameFunctionFrontendActionFactory.hpp"
//#include "CompletionResultListToPythonList.hpp"
#include "Session.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>

struct PythonContext
{
	PythonContext()
	{
		Py_Initialize();
		PyEval_InitThreads();
	}

	~PythonContext()
	{
		Py_Finalize();
	}
};

int main(int argc, const char** argv)
{
	if (argc < 4)
	{
		std::cerr << "Usage: " << argv[0] << " <file> <line> <column>\n";
		return EXIT_FAILURE;
	}

	PythonContext context;

	//boost::python::to_python_converter<std::vector<std::pair<std::string, std::string>>, Clara::CompletionResultListToPythonList>();
	clang::TextDiagnosticPrinter consumer(llvm::errs(), nullptr);

	const int line = std::atoi(argv[2]);
	const int column = std::atoi(argv[3]);
	std::unique_ptr<Clara::Session> session;
	try
	{
		#ifdef __APPLE__
		session.reset(new Clara::Session(consumer, argv[1], "/Users/rwols/Library/Application Support/Sublime Text 3/Packages/clara/build"));
		#else
		session.reset(new Clara::Session(consumer, argv[1], "/home/raoul/.config/sublime-text-3/Packages/Clara/build"));
		#endif
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		try
		{
			session.reset(new Clara::Session(argv[1]));
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return EXIT_FAILURE;
		}
	}

	session->reporter = boost::python::object(+[] (const std::string& message) -> void
	{
		std::cout << message << '\n';
	});

	// boost::python::object callback(+[] (boost::python::list results) -> void
	// {
	// 	std::cout << "Got results:\n";
	// 	for (int i = 0; i < boost::python::len(results); ++i)
	// 	{
	// 		std::string first = boost::python::extract<std::string>(results[i][0]);
	// 		std::string second = boost::python::extract<std::string>(results[i][1]);
	// 		std::cout << first << ", " << second << '\n';
	// 	}
	// });

	std::string unsavedBuffer;
	{
		std::ifstream input(argv[1]);	
		input >> unsavedBuffer;
	}
	
	try
	{
		std::cout << "Code completing at line " << line << ", column " << column << '\n';
		auto results = session->codeComplete(unsavedBuffer.c_str(), line, column);
		for (const auto& result : results)
		{
			std::cout << result.first << " => " << result.second << '\n';
		}
	}
	catch (const boost::python::error_already_set& e)
	{
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);
		if (PyUnicode_Check(pvalue)) 
		{
			PyObject * tempBytes = PyUnicode_AsEncodedString(pvalue, "ASCII", "strict"); // Owned reference
			if (tempBytes != NULL) 
			{
				std::cerr << PyBytes_AS_STRING(tempBytes) << '\n';
				Py_DECREF(tempBytes);
			}
		}
    }

	return EXIT_SUCCESS;
}