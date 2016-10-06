#include <iostream>
#include <cstddef>
#include <memory>
#include <string>

#include <llvm/Support/Casting.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>

#include <clang/Rewrite/Core/Rewriter.h>

using namespace llvm;
using namespace clang;

int numFunctions = 0;

class ExampleVisitor : public clang::RecursiveASTVisitor<ExampleVisitor>
{
private:
	
	clang::ASTContext *mContext;
	clang::Rewriter* mRewriter;
	int mNumFunctions = 0;

public:
	explicit ExampleVisitor(clang::CompilerInstance *compiler, clang::Rewriter* rewriter)
	: mContext(&(compiler->getASTContext()))
	, mRewriter(rewriter)
	{
		mRewriter->setSourceMgr(mContext->getSourceManager(), mContext->getLangOpts());
	}
 
	virtual bool VisitFunctionDecl(FunctionDecl *func) 
	{
		mNumFunctions++;
		++numFunctions;
		std::string funcName = func->getNameInfo().getName().getAsString();
		if (funcName == "do_math") 
		{
			mRewriter->ReplaceText(func->getLocation(), funcName.length(), "add5");
			errs() << "** Rewrote function def: " << funcName << "\n";         
		}         
		return true;     
	}     
	 
	virtual bool VisitStmt(Stmt *st) {
		if (ReturnStmt *ret = llvm::dyn_cast<ReturnStmt>(st)) 
		{
			mRewriter->ReplaceText(ret->getRetValue()->getLocStart(), 6, "val");
			errs() << "** Rewrote ReturnStmt\n";
		}
		if (CallExpr *call = llvm::dyn_cast<CallExpr>(st)) 
		{
			mRewriter->ReplaceText(call->getLocStart(), 7, "add5");
			errs() << "** Rewrote function call\n";
		}
		return true;
	}
};

class ExampleASTConsumer : public ASTConsumer 
{
private:

	ExampleVisitor *visitor; // doesn't have to be private.

public:
	
	explicit ExampleASTConsumer(clang::CompilerInstance *compiler)
	: visitor(new ExampleVisitor(compiler))
	{
		/* Empty for now */
	}
 
	virtual bool HandleTopLevelDecl(DeclGroupRef declgroupref)
	{
		// a DeclGroupRef may have multiple Decls, so we iterate through each one
		
		for (auto* decl : declgroupref)
		{
			visitor->TraverseDecl(decl);
		}

		return true;
	}

	virtual void HandleTranslationUnit(ASTContext& context)
	{
		visitor->TraverseDecl(context.getTranslationUnitDecl());
	}
};

class ExampleFrontendAction : public clang::ASTFrontendAction 
{
private:

	Rewriter* mRewriter = nullptr;

public:

	void setRewriter(Rewriter* rewriter)
	{
		mRewriter = rewriter;
	}

	Rewriter* getRewriter()
	{
		return mRewriter;
	}

	const Rewriter* getRewriter() const
	{
		return mRewriter;
	}
	
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef inFile)
	{
		return std::unique_ptr<clang::ASTConsumer>(new ExampleASTConsumer(&compiler, mRewriter));
	}
};

int main(const int argc, const char** argv)
{
	int numFunctions = 0; 

	// parse the command-line args passed to your code
	CommonOptionsParser options(argc, argv);
	// create a new Clang Tool instance (a LibTooling environment)
	ClangTool tool(options.getCompilations(), options.getSourcePathList());

	Rewriter rewriter;

	auto factory = std::make_unique<FrontendActionFactory<ExampleFrontendAction>>();
	auto action = factory->create();
	if (auto example = dyn_cast<ExampleFrontendAction>(action))
	{
		example->setRewriter(&rewriter);
	}

	// run the Clang Tool, creating a new FrontendAction (explained below)
	int result = tool.run(new FrontendActionFactory<ExampleFrontendAction>());

	errs() << "\nFound " << numFunctions << " functions.\n\n";
	// print out the rewritten source code ("rewriter" is a global var.)
	rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(errs());
	return result;
	
	return EXIT_SUCCESS;
}