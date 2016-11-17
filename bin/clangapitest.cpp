#include <clang/Frontend/CompilerInstance.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Basic/TargetInfo.h>
#include <memory>
#include <iostream>

const char* FILENAME = "/Users/rwols/Library/Application Support/Sublime Text 3/Packages/clara/test/test2.cpp";
const int ROW = 65;
const int COL = 10;

#ifdef __APPLE__

const char* standardIncludes[] =
{
	"/usr/include/c++/4.2.1/",
	"/usr/include",
	"/usr/local/include",
	"/usr/local/lib/clang/4.0.0/include/"
};

#else

const char* standardIncludes[] =
{
	"/usr/include/c++/5.4.0",
	"/usr/include/x86_64-linux-gnu/c++/5.4.0",
	"/usr/include/c++/5.4.0/backward",
	"/usr/local/include",
	"/usr/local/lib/clang/4.0.0/include",
	"/usr/include/x86_64-linux-gnu",
	"/usr/include"
};

#endif

int main()
{
	using namespace clang;

	CompilerInstance instance;
	instance.createDiagnostics();

	// Setup target, filemanager and sourcemanager
	auto targetOptions = std::make_shared<TargetOptions>();
	targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
	instance.setTarget(TargetInfo::CreateTargetInfo(instance.getDiagnostics(), targetOptions));
	instance.createFileManager();
	instance.createSourceManager(instance.getFileManager());
	CompilerInvocation::setLangDefaults(instance.getLangOpts(), IK_CXX, llvm::Triple(targetOptions->Triple), instance.getPreprocessorOpts());

	// Create code completion consumer
	CodeCompleteOptions codeCompleteOptions;
	codeCompleteOptions.IncludeMacros = false;
	codeCompleteOptions.IncludeCodePatterns = false;
	codeCompleteOptions.IncludeGlobals = false;
	codeCompleteOptions.IncludeBriefComments = false;
	instance.setCodeCompletionConsumer(new PrintingCodeCompleteConsumer(codeCompleteOptions, llvm::outs()));

	// Setup header search paths
	instance.createPreprocessor(TranslationUnitKind::TU_Complete);
	auto& preprocessor = instance.getPreprocessor();
	auto& headerSearch = preprocessor.getHeaderSearchInfo();
	auto& headerSearchOpts = instance.getHeaderSearchOpts();
	headerSearchOpts.ResourceDir = "/usr/local/lib/clang/4.0.0/include/";
	std::vector<DirectoryLookup> lookups;
	auto& fileManager = instance.getFileManager();
	for (const auto standardInclude : standardIncludes)
	{
		headerSearchOpts.UserEntries.emplace_back(standardInclude, frontend::IncludeDirGroup::System, false, false);
		lookups.emplace_back(fileManager.getDirectory(standardInclude), SrcMgr::CharacteristicKind::C_System, false);
	}
	headerSearch.SetSearchPaths(lookups, 0, 0, true);

	auto& frontendOptions = instance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.Line = ROW;
	frontendOptions.CodeCompletionAt.Column = COL;
	frontendOptions.CodeCompletionAt.FileName = FILENAME;
	FrontendInputFile input(FILENAME, InputKind::IK_CXX);
	frontendOptions.Inputs.push_back(input);

	SyntaxOnlyAction action;
	std::vector<std::pair<std::string, std::string>> results;
	if (action.BeginSourceFile(instance, frontendOptions.Inputs[0]))
	{
		action.Execute();
		action.EndSourceFile();
	}

	return 0;
}