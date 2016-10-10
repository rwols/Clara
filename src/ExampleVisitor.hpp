#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Frontend/CompilerInstance.h>
#include <string>

class ExampleVisitor : public clang::RecursiveASTVisitor<ExampleVisitor>
{
private:
	
	clang::ASTContext *mContext;
	clang::Rewriter* mRewriter;
	int mNumFunctions = 0;

public:
	explicit ExampleVisitor(clang::CompilerInstance *compiler, clang::Rewriter* rewriter);
 
	bool VisitFunctionDecl(clang::FunctionDecl *func);
	 
	bool VisitStmt(clang::Stmt *st);

	int getNumFunctions() const;
};