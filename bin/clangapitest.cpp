#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <iostream>

// The filename that will be processed (twice).
static const char* FILENAME = "simple.cpp";

// System header locations, you may need to
// adjust these.
static const char* SYSTEM_HEADERS[] =
{
	"/usr/include/c++/5.4.0",
	"/usr/include/x86_64-linux-gnu/c++/5.4.0",
	"/usr/include/c++/5.4.0/backward",
	"/usr/local/lib/clang/4.0.0/include",
	"/usr/include/x86_64-linux-gnu",
	"/usr/include"
};

// Location for builtin headers. You may need to
// adjust this.
static const char* RESOURCE_DIR = "/usr/local/lib/clang/4.0.0";

// Uncomment this to see header search paths.
// #define PRINT_HEADER_SEARCH_PATHS

// Constructs a CompilerInvocation
// that must be fed to a CompilerInstance.
clang::CompilerInvocation* makeInvocation();

// Executes a single SyntaxOnlyAction on
// the given CompilerInstance.
void secondCallThisFunctionFails(clang::CompilerInstance& instance);

void makeInstanceAndExecuteAction();

int main()
{
	// makeInstanceAndExecuteAction();
	// makeInstanceAndExecuteAction();
	using namespace clang;

	CompilerInstance instance;
	
	instance.createDiagnostics();

	instance.setInvocation(makeInvocation());
	instance.getFrontendOpts().Inputs.emplace_back
	(
		FILENAME, 
		FrontendOptions::getInputKindForExtension(FILENAME)
	);

	// First call is OK.
	secondCallThisFunctionFails(instance);

	// Second call results in assertion failures.
	secondCallThisFunctionFails(instance);

	return 0;
}

clang::CompilerInvocation* makeInvocation()
{
	using namespace clang;
	auto invocation = new CompilerInvocation();

	invocation->TargetOpts->Triple = llvm::sys::getDefaultTargetTriple();
	invocation->setLangDefaults(
		*invocation->getLangOpts(), 
		IK_CXX, 
		llvm::Triple(invocation->TargetOpts->Triple), 
		invocation->getPreprocessorOpts(), 
		LangStandard::lang_cxx11);

	auto& headerSearchOpts = invocation->getHeaderSearchOpts();

	#ifdef PRINT_HEADER_SEARCH_PATHS
		headerSearchOpts.Verbose = true;
	#else
		headerSearchOpts.Verbose = false;
	#endif

	headerSearchOpts.UseBuiltinIncludes = true;
	headerSearchOpts.UseStandardSystemIncludes = true;
	headerSearchOpts.UseStandardCXXIncludes = true;
	headerSearchOpts.ResourceDir = RESOURCE_DIR;

	for (const auto sytemHeader : SYSTEM_HEADERS)
	{
		headerSearchOpts.AddPath(sytemHeader, frontend::System, false, false);
	}

	return invocation;
}

void secondCallThisFunctionFails(clang::CompilerInstance& instance)
{
	using namespace clang;
	instance.setSourceManager(nullptr);
	instance.createDiagnostics();
	SyntaxOnlyAction action;
	if (instance.ExecuteAction(action))
	{
		std::cout << "Action succeeded.\n";
	}
	else
	{
		std::cout << "Action failed.\n";
	}
}

void makeInstanceAndExecuteAction()
{
	using namespace clang;

	CompilerInstance instance;
	
	instance.createDiagnostics();

	instance.setInvocation(makeInvocation());
	instance.getFrontendOpts().Inputs.emplace_back
	(
		FILENAME, 
		FrontendOptions::getInputKindForExtension(FILENAME)
	);

	// First call is OK.
	secondCallThisFunctionFails(instance);
}