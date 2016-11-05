#include "RenameFunctionFrontendAction.hpp"
#include "RenameFunctionASTConsumer.hpp"

using namespace llvm;
using namespace clang;

std::unique_ptr<clang::ASTConsumer> RenameFunctionFrontendAction::CreateASTConsumer(
	clang::CompilerInstance &compiler, 
	llvm::StringRef inFile)
{
	return std::make_unique<RenameFunctionASTConsumer>(&compiler, rewriter, oldFullyQualifiedName, newFullyQualifiedName);
}