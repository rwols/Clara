#pragma once

#include "PyBind11.hpp"
#include <clang/Basic/Diagnostic.h>

namespace Clara
{

class DiagnosticConsumer : public clang::DiagnosticConsumer
{
  public:
    DiagnosticConsumer(pybind11::object callback);
    ~DiagnosticConsumer() override = default;
    void BeginSourceFile(const clang::LangOptions &LangOpts,
                         const clang::Preprocessor *preprocessor) override;
    void HandleDiagnostic(clang::DiagnosticsEngine::Level level,
                          const clang::Diagnostic &info) override;
    void finish() override;

  private:
    pybind11::object mCallback;
    clang::SourceManager *mSourceMgr = nullptr;
    clang::PresumedLoc makePresumedLoc(const clang::Diagnostic &info) const;
    void doCallback(const char *messageType,
                    const clang::PresumedLoc &presumedLoc, const char *message);
    void doCallback(const char *filename, const char *messageType, int row,
                    int column, const char *message);
};

} // namespace Clara
