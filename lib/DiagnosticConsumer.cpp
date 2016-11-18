#include "DiagnosticConsumer.hpp"

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
	boost::python::list result;
	for (unsigned i = 0; i < info.getNumArgs(); ++i)
	{
		switch (info.getArgKind(i))
		{
			case clang::DiagnosticsEngine::ak_std_string:
				result.append(info.getArgStdStr(i));
				break;
			case clang::DiagnosticsEngine::ak_c_string:
				result.append(info.getArgCStr(i));
				break;
			case clang::DiagnosticsEngine::ak_sint:
				result.append(info.getArgSInt(i));
				break;
			case clang::DiagnosticsEngine::ak_uint:
				result.append(info.getArgUInt(i));
				break;
			default:
				result.append("unkown type");
				break;
		}
		result.append(info.getArgStdStr(i));
	}
	handleDiagnostic(level, result);
}

void DiagnosticConsumer::beginSourceFile()
{
	// do nothing
}

void DiagnosticConsumer::handleDiagnostic(clang::DiagnosticsEngine::Level level, boost::python::list info)
{
	// do nothing
}

} // namespace Clara