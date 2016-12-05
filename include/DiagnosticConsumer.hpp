#pragma once

#include "PyBind11.hpp"
#include <clang/Basic/Diagnostic.h>

namespace Clara {

class DiagnosticConsumer 
: public clang::DiagnosticConsumer
, public std::enable_shared_from_this<DiagnosticConsumer>
{
public:
	virtual ~DiagnosticConsumer() override = default;
	void BeginSourceFile(const clang::LangOptions& LangOpts, const clang::Preprocessor* preprocessor) override;
	void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override;
	virtual void handleNote(const std::string& filename, int row, int column, const std::string& message) = 0;
	virtual void handleRemark(const std::string& filename, int row, int column, const std::string& message) = 0;
	virtual void handleWarning(const std::string& filename, int row, int column, const std::string& message) = 0;
	virtual void handleError(const std::string& filename, int row, int column, const std::string& message) = 0;
	virtual void handleFatalError(const std::string& filename, int row, int column, const std::string& message) = 0;
private:
	clang::SourceManager* mSourceMgr = nullptr;
	clang::PresumedLoc makePresumedLoc(const clang::Diagnostic& info) const;
};

class PyDiagnosticConsumer : DiagnosticConsumer
{
public:
	~PyDiagnosticConsumer() override = default;
	void handleNote(const std::string& filename, int row, int column, const std::string& message) override
	{
		PYBIND11_OVERLOAD_PURE(void, DiagnosticConsumer, handleNote, filename, row, column, message);
	}
	void handleRemark(const std::string& filename, int row, int column, const std::string& message) override
	{
		PYBIND11_OVERLOAD_PURE(void, DiagnosticConsumer, handleRemark, filename, row, column, message);
	}
	void handleWarning(const std::string& filename, int row, int column, const std::string& message) override
	{
		PYBIND11_OVERLOAD_PURE(void, DiagnosticConsumer, handleWarning, filename, row, column, message);
	}
	void handleError(const std::string& filename, int row, int column, const std::string& message) override
	{
		PYBIND11_OVERLOAD_PURE(void, DiagnosticConsumer, handleError, filename, row, column, message);
	}
	void handleFatalError(const std::string& filename, int row, int column, const std::string& message) override
	{
		PYBIND11_OVERLOAD_PURE(void, DiagnosticConsumer, handleFatalError, filename, row, column, message);
	}
};

} // namespace Clara