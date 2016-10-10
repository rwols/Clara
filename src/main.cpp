#include <iostream>
#include <cstddef>
#include <memory>
#include <string>

#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

#include "ExampleFrontendAction.hpp"
#include "ExampleFrontendActionFactory.hpp"
#include "ExampleVisitor.hpp"
#include "ExampleASTConsumer.hpp"

int main(int argc, const char** argv)
{
	llvm::cl::OptionCategory category("Tooling");

	// parse the command-line args passed to your code
	clang::tooling::CommonOptionsParser options(argc, argv, category);
	// create a new Clang Tool instance (a LibTooling environment)
	clang::tooling::ClangTool tool(options.getCompilations(), options.getSourcePathList());

	clang::Rewriter rewriter;
	ExampleFrontendActionFactory factory(&rewriter);
	int result = tool.run(&factory);
	return result;
	
	// return 0;

}