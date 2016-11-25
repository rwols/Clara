#pragma once

#include <pybind11/pybind11.h>
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
	virtual void handleDiagnostic(clang::DiagnosticsEngine::Level level, pybind11::list info);
};

} // namespace Clara