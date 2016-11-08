#include "RenameFunctionASTConsumer.hpp"

RenameFunctionASTConsumer::RenameFunctionASTConsumer(
	clang::CompilerInstance *compiler, 
	clang::Rewriter* rewriter,
	const std::string& oldName,
	const std::string& newName)
: mVisitor(new RenameFunctionVisitor(compiler, rewriter, oldName, newName))
{
	/* Empty for now */
}

void RenameFunctionASTConsumer::HandleTranslationUnit(clang::ASTContext& context)
{
	mVisitor->TraverseDecl(context.getTranslationUnitDecl());
}

const RenameFunctionVisitor* RenameFunctionASTConsumer::getVisitor() const
{
	return mVisitor;
}