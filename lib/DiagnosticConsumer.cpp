#include "DiagnosticConsumer.hpp"
#include <pybind11/stl.h>
#include <llvm/ADT/SmallString.h>
#include <clang/Lex/Preprocessor.h>

namespace Clara {

DiagnosticConsumer::DiagnosticConsumer(pybind11::object callback)
: mCallback(std::move(callback))
{
	
}

void DiagnosticConsumer::BeginSourceFile(const clang::LangOptions &options, const clang::Preprocessor* pp)
{
	clang::DiagnosticConsumer::BeginSourceFile(options, pp);
	mSourceMgr = pp == nullptr ? nullptr : &(pp->getSourceManager());
	doCallback("", "begin", -1, -1, "");
}

void DiagnosticConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info)
{
	clang::DiagnosticConsumer::HandleDiagnostic(level, info);
	llvm::SmallString<128> message;
	info.FormatDiagnostic(message);
	const auto presumedLoc = makePresumedLoc(info);
	switch (level)
	{
		case clang::DiagnosticsEngine::Note:
			doCallback("note", presumedLoc, message.c_str());
			break;
		case clang::DiagnosticsEngine::Remark:
			doCallback("remark", presumedLoc, message.c_str());
			break;
		case clang::DiagnosticsEngine::Warning:
			doCallback("warning", presumedLoc, message.c_str());
			break;
		case clang::DiagnosticsEngine::Error:
			doCallback("error", presumedLoc, message.c_str());
			break;
		case clang::DiagnosticsEngine::Fatal:
			doCallback("fatal", presumedLoc, message.c_str());
			break;
		default:
			break;
	}
}

void DiagnosticConsumer::finish()
{
	clang::DiagnosticConsumer::finish();
	doCallback("", "finish", -1, -1, "");
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

void DiagnosticConsumer::doCallback(const char* messageType, const clang::PresumedLoc& presumedLoc, const char* message)
{
	if (presumedLoc.isValid())
	{
		doCallback(presumedLoc.getFilename(), messageType, presumedLoc.getLine(), presumedLoc.getColumn(), message);
	}
	else
	{
		doCallback("", messageType, -1, -1, message);
	}
}

void DiagnosticConsumer::doCallback(const char* filename, const char* messageType, int row, int column, const char* message)
{
	pybind11::gil_scoped_acquire pythonLock;
	if (mCallback != pybind11::object())
	{
		try
		{
			mCallback(filename, messageType, row, column, message);
		}
		catch (const std::exception& err)
		{
			// oops... now what?
		}
	}
}

} // namespace Clara