#include "ExampleFrontendAction.hpp"
#include "ExampleASTConsumer.hpp"

using namespace llvm;
using namespace clang;

void ExampleFrontendAction::setRewriter(Rewriter* rewriter)
{
	mRewriter = rewriter;
}

Rewriter* ExampleFrontendAction::getRewriter()
{
	return mRewriter;
}

const Rewriter* ExampleFrontendAction::getRewriter() const
{
	return mRewriter;
}
	
std::unique_ptr<clang::ASTConsumer> ExampleFrontendAction::CreateASTConsumer(
	clang::CompilerInstance &compiler, 
	llvm::StringRef inFile)
{
	return std::make_unique<ExampleASTConsumer>(&compiler, mRewriter);
}