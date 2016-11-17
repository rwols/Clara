#pragma once

#include <clang/AST/ASTConsumer.h>

#include "RenameFunctionVisitor.hpp"

class RenameFunctionASTConsumer : public clang::ASTConsumer 
{
private:

	RenameFunctionVisitor *mVisitor; // doesn't have to be private.

public:
	const RenameFunctionVisitor* getVisitor() const;
	explicit RenameFunctionASTConsumer(clang::CompilerInstance *compiler, clang::Rewriter* rewriter, const std::string& oldName, const std::string& newName);
	void HandleTranslationUnit(clang::ASTContext& context) override;
};
