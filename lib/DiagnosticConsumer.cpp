#include "DiagnosticConsumer.hpp"
#include <pybind11/stl.h>

namespace Clara {

void DiagnosticConsumer::BeginSourceFile(const clang::LangOptions &options, const clang::Preprocessor* pp)
{
	beginSourceFile();
}

void DiagnosticConsumer::EndSourceFile()
{
	// do nothing
}

void DiagnosticConsumer::finish()
{

	// do nothing
}

bool DiagnosticConsumer::IncludeInDiagnosticCounts() const
{
	return true;
}

void DiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info)
{
	pybind11::list result;
	for (unsigned i = 0; i < info.getNumArgs(); ++i)
	{
		switch (info.getArgKind(i))
		{
			case clang::DiagnosticsEngine::ak_std_string:
				result.append(pybind11::cast(info.getArgStdStr(i)));
				break;
			case clang::DiagnosticsEngine::ak_c_string:
				result.append(pybind11::cast(info.getArgCStr(i)));
				break;
			case clang::DiagnosticsEngine::ak_sint:
				result.append(pybind11::cast(info.getArgSInt(i)));
				break;
			case clang::DiagnosticsEngine::ak_uint:
				result.append(pybind11::cast(info.getArgUInt(i)));
				break;
			default:
				result.append(pybind11::cast("unkown type"));
				break;
		}
	}
	handleDiagnostic(level, result);
}

void DiagnosticConsumer::beginSourceFile()
{
	// do nothing
}

void DiagnosticConsumer::handleDiagnostic(clang::DiagnosticsEngine::Level level, pybind11::list info)
{
	// do nothing
}

} // namespace Clara