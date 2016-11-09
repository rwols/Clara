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