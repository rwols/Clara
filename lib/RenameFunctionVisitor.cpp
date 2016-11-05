#include "RenameFunctionVisitor.hpp"
#include <llvm/Support/Casting.h>
#include <iostream>

RenameFunctionVisitor::RenameFunctionVisitor(
	clang::CompilerInstance *compiler, 
	clang::Rewriter* rewriter, 
	const std::string& oldFullyQualifiedName, 
	const std::string& newFullyQualifiedName)
: mContext(&(compiler->getASTContext()))
, mRewriter(rewriter)
, mOldName(oldFullyQualifiedName)
, mNewName(newFullyQualifiedName)
{
	mRewriter->setSourceMgr(mContext->getSourceManager(), mContext->getLangOpts());
}
 
bool RenameFunctionVisitor::VisitFunctionDecl(clang::FunctionDecl *func) 
{
	mNumFunctions++;
	const auto funcName = func->getQualifiedNameAsString();
	if (funcName == mOldName) 
	{
		auto length = func->getNameInfo().getName().getAsString().length();
		mRewriter->ReplaceText(func->getLocation(), length, mNewName);
	}
	return true;
}

bool RenameFunctionVisitor::VisitCallExpr(clang::CallExpr* expr)
{
	auto func = expr->getDirectCallee();
	if (func == nullptr) return true; // continue traversing
	const auto funcName = func->getQualifiedNameAsString();
	if (funcName == mOldName) 
	{
		auto length = funcName.length();
		mRewriter->ReplaceText(expr->getLocStart(), length, mNewName);
	}
	return true;
}

bool RenameFunctionVisitor::VisitDeclRefExpr(clang::DeclRefExpr* expr)
{
	const auto decl = expr->getDecl();
	if (decl == nullptr) return true; // continue traversing
	const auto funcDecl = llvm::dyn_cast<const clang::FunctionDecl>(decl);
	if (funcDecl == nullptr) return true; // continue traversing
	const auto funcName = funcDecl->getQualifiedNameAsString();
	if (funcName == mOldName)
	{
		mRewriter->ReplaceText(expr->getLocation(), funcName.length(), mNewName);
	}
	return true;
}

bool RenameFunctionVisitor::VisitStmt(clang::Stmt *st) 
{
	return true;
}

int RenameFunctionVisitor::getNumFunctions() const
{
	return mNumFunctions;
}
