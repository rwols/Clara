#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include "RenameFunctionFrontendActionFactory.hpp"

int main(int argc, const char** argv)
{

	llvm::cl::OptionCategory category("Tooling");

	// parse the command-line args passed to your code
	clang::tooling::CommonOptionsParser options(argc, argv, category);
	// create a new Clang Tool instance (a LibTooling environment)
	clang::tooling::ClangTool tool(options.getCompilations(), options.getSourcePathList());

	clang::Rewriter rewriter;
	RenameFunctionFrontendActionFactory factory;
	factory.rewriter = &rewriter;
	factory.oldFullyQualifiedName = "A::somefunc";
	factory.newFullyQualifiedName = "FOOBAR";
	int result = tool.run(&factory);

	rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::errs());

	return result;
	
	// return 0;

}