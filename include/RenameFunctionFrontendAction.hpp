#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include "RenameFunctionASTConsumer.hpp"

class RenameFunctionFrontendAction : public clang::ASTFrontendAction 
{
public:

	clang::Rewriter* rewriter = nullptr;
	std::string oldFullyQualifiedName;
	std::string newFullyQualifiedName;

	RenameFunctionFrontendAction() = default;
	RenameFunctionFrontendAction(clang::Rewriter* rewriter);
	
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &compiler, 
		llvm::StringRef inFile) override;
};