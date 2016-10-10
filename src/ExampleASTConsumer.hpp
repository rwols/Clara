#pragma once

#include <clang/AST/ASTConsumer.h>

#include "ExampleVisitor.hpp"

class ExampleASTConsumer : public clang::ASTConsumer 
{
private:

	ExampleVisitor *mVisitor; // doesn't have to be private.

public:
	const ExampleVisitor* getVisitor() const;
	explicit ExampleASTConsumer(clang::CompilerInstance *compiler, clang::Rewriter* rewriter);
	virtual bool HandleTopLevelDecl(clang::DeclGroupRef declgroupref);
	virtual void HandleTranslationUnit(clang::ASTContext& context);
};
