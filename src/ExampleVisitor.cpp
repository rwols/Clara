#include "ExampleVisitor.hpp"
#include <llvm/Support/Casting.h>
#include <iostream>

ExampleVisitor::ExampleVisitor(clang::CompilerInstance *compiler, clang::Rewriter* rewriter)
: mContext(&(compiler->getASTContext()))
, mRewriter(rewriter)
{
	if (mRewriter == nullptr)
	{
		std::cerr << "Oops!\n";
	}
	mRewriter->setSourceMgr(mContext->getSourceManager(), mContext->getLangOpts());
}
 
bool ExampleVisitor::VisitFunctionDecl(clang::FunctionDecl *func) 
{
	mNumFunctions++;
	std::string funcName = func->getNameInfo().getName().getAsString();
	llvm::errs() << "Function declaration: " << funcName <<'\n';
	// if (funcName == "do_math") 
	// {
	// 	mRewriter->ReplaceText(func->getLocation(), funcName.length(), "add5");
	// 	errs() << "** Rewrote function def: " << funcName << "\n";         
	// }         
	return true;
}
	 
bool ExampleVisitor::VisitStmt(clang::Stmt *st) 
{
	// if (clang::ReturnStmt *ret = llvm::dyn_cast<clang::ReturnStmt>(st)) 
	// {
	// 	llvm::errs() << "Return statement...\n";
	// }
	// else if (clang::CallExpr *call = llvm::dyn_cast<clang::CallExpr>(st)) 
	// {
	// 	llvm::errs() << "Call expression...\n";
	// }
	// else
	// {
	// 	llvm::errs() << "Statement...\n";
	// }
	return true;
}

int ExampleVisitor::getNumFunctions() const { return mNumFunctions; }
