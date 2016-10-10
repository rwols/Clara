#include "ExampleASTConsumer.hpp"

ExampleASTConsumer::ExampleASTConsumer(clang::CompilerInstance *compiler, clang::Rewriter* rewriter)
: mVisitor(new ExampleVisitor(compiler, rewriter))
{
	/* Empty for now */
}
 
bool ExampleASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef declgroupref)
{
	// a DeclGroupRef may have multiple Decls, so we iterate through each one
	
	for (auto* decl : declgroupref)
	{
		mVisitor->TraverseDecl(decl);
	}

	return true;
}

void ExampleASTConsumer::HandleTranslationUnit(clang::ASTContext& context)
{
	mVisitor->TraverseDecl(context.getTranslationUnitDecl());
}

const ExampleVisitor* ExampleASTConsumer::getVisitor() const
{
	return mVisitor;
}