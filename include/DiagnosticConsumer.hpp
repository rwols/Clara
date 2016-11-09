#pragma once

#include <boost/python.hpp>
#include <clang/Basic/Diagnostic.h>

namespace Clara {

class DiagnosticConsumer : public clang::DiagnosticConsumer
{
public:
	void BeginSourceFile(const clang::LangOptions& options, const clang::Preprocessor* pp) override;
	void EndSourceFile() override;
	void finish() override;
	bool IncludeInDiagnosticCounts() const override; 
	void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override;

	virtual void beginSourceFile();
	virtual void handleDiagnostic(clang::DiagnosticsEngine::Level level, boost::python::list info);
};

} // namespace Clara