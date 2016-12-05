#include "DiagnosticConsumer.hpp"
#include <pybind11/stl.h>
#include <llvm/ADT/SmallString.h>
#include <clang/Lex/Preprocessor.h>

namespace Clara {

void DiagnosticConsumer::BeginSourceFile(const clang::LangOptions &options, const clang::Preprocessor* pp)
{
	clang::DiagnosticConsumer::BeginSourceFile(options, pp);
	mSourceMgr = pp == nullptr ? nullptr : &(pp->getSourceManager());
}

void DiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info)
{
	clang::DiagnosticConsumer::HandleDiagnostic(level, info);
	llvm::SmallString<100> message;
	info.FormatDiagnostic(message);
	#ifndef NDEBUG
	llvm::errs() << "DIAG MSG: " << message << '\n';
	#endif // NDEBUG
	const auto presumedLoc = makePresumedLoc(info);
	switch (level) 
	{
		case clang::DiagnosticsEngine::Note:
			if (presumedLoc.isValid()) handleNote(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn(), message.c_str());
			else handleNote(std::string(), -1, -1, message.c_str());
			break;
		case clang::DiagnosticsEngine::Warning:
			if (presumedLoc.isValid()) handleWarning(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn(), message.c_str());
			else handleWarning(std::string(), -1, -1, message.c_str());
			break;
		case clang::DiagnosticsEngine::Remark:
			if (presumedLoc.isValid()) handleRemark(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn(), message.c_str());
			else handleRemark(std::string(), -1, -1, message.c_str());
			break;
		case clang::DiagnosticsEngine::Error:
			if (presumedLoc.isValid()) handleError(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn(), message.c_str());
			else handleError(std::string(), -1, -1, message.c_str());
			break;
		case clang::DiagnosticsEngine::Fatal:
			if (presumedLoc.isValid()) handleFatalError(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn(), message.c_str());
			else handleFatalError(std::string(), -1, -1, message.c_str());
			break;
		default:
			break;
	}
}

clang::PresumedLoc DiagnosticConsumer::makePresumedLoc(const clang::Diagnostic& info) const
{
	if (mSourceMgr)
	{
		return mSourceMgr->getPresumedLoc(info.getLocation());
	}
	else
	{
		return clang::PresumedLoc(nullptr, -1, -1, clang::SourceLocation());
	}
}

} // namespace Clara