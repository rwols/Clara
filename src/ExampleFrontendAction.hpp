#pragma once

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include "ExampleASTConsumer.hpp"

class ExampleFrontendAction : public clang::ASTFrontendAction 
{
private:

	clang::Rewriter* mRewriter = nullptr;

public:

	ExampleFrontendAction() = default;
	ExampleFrontendAction(clang::Rewriter* rewriter);
	virtual ~ExampleFrontendAction() noexcept = default;

	void setRewriter(clang::Rewriter* rewriter);

	clang::Rewriter* getRewriter();

	const clang::Rewriter* getRewriter() const;

	const ExampleASTConsumer* getConsumer() const;
	
	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &compiler, 
		llvm::StringRef inFile) override;
};