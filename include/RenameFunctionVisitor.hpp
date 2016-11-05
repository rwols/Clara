#pragma once

#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Frontend/CompilerInstance.h>
#include <string>

class RenameFunctionVisitor : public clang::RecursiveASTVisitor<RenameFunctionVisitor>
{
private:
	
	clang::ASTContext *mContext;
	clang::Rewriter* mRewriter;
	int mNumFunctions = 0;
	const std::string mOldName;
	const std::string mNewName;

public:
	
	explicit RenameFunctionVisitor(
		clang::CompilerInstance *compiler, 
		clang::Rewriter* rewriter, 
		const std::string& oldFullyQualifiedName, 
		const std::string& newFullyQualifiedName);
 
	bool VisitFunctionDecl(clang::FunctionDecl* func);
	bool VisitCallExpr(clang::CallExpr* expr);
	bool VisitDeclRefExpr(clang::DeclRefExpr* expr);
	 
	bool VisitStmt(clang::Stmt *st);

	int getNumFunctions() const;
};