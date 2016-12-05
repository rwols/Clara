// This file is meant to become a precompiled header (PCH).
// You put here those headers which will probably never
// change during the course of the project, or rarely.
#pragma once

// Pybind11 headers
#ifdef __clang__
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif // __clang__
#include <pybind11/pybind11.h>
#ifdef __clang__
#	pragma clang diagnostic pop
#endif // __clang__
#include <pybind11/stl.h>

// LLVM headers
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Casting.h>

// Clang headers
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/TargetInfo.h>

#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/DeclGroup.h>

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Overload.h>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/LangStandard.h>

#include <clang/Rewrite/Core/Rewriter.h>

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/CommonOptionsParser.h>

// C++ standard headers
#include <condition_variable>
#include <stdexcept>
#include <string>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <mutex>
#include <thread>
