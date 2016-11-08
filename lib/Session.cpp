#include "Session.hpp"
#include <llvm/Support/CommandLine.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Basic/TargetInfo.h>
#include "RenameFunctionFrontendActionFactory.hpp"
#include "CustomCodeCompleteConsumer.hpp"

#include <iostream>

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

#define DEBUG_PRINT llvm::errs() << __LINE__ << '\n'

std::string Session::loadCompilationDatabase(const std::string& compilationDatabaseFile)
{
	using namespace clang;
	using namespace clang::tooling;

	std::string errorMessage;
	mCompilationDatabase = JSONCompilationDatabase::loadFromFile(compilationDatabaseFile, errorMessage, JSONCommandLineSyntax::AutoDetect);
	return errorMessage;
}

bool Session::hasCompilationDatabase() const noexcept
{
	return mCompilationDatabase != nullptr;
}

void Session::setSourcePaths(const boost::python::list& sourcePaths)
{
	using namespace boost;

	mSourcePaths.clear();
	for (int i = 0; i < python::len(sourcePaths); ++i)
	{
		mSourcePaths.push_back(python::extract<std::string>(sourcePaths[i]));
	}
}

void Session::reloadTool()
{
	using namespace llvm;
	using namespace clang::tooling;

	ArrayRef<std::string> sourcePaths(mSourcePaths.data(), mSourcePaths.data() + mSourcePaths.size());
	const auto& compDatabase = *(mCompilationDatabase.get());
	mTool = std::make_unique<ClangTool>(compDatabase, sourcePaths);
	mSyntaxTrees.clear();
	mTool->buildASTs(mSyntaxTrees);
}

boost::python::list Session::codeComplete(const std::string& filename, const char* buffer, int row, int column)
{
	using namespace clang;
	using namespace boost;

	// Setup
	CompilerInstance instance;
	auto &langOptions = instance.getLangOpts();
	langOptions.CPlusPlus = true;
	langOptions.CPlusPlus11 = true;
	instance.createDiagnostics();
	auto targetOptions = std::make_shared<TargetOptions>();
	targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
	instance.setTarget(TargetInfo::CreateTargetInfo(instance.getDiagnostics(), targetOptions));
	instance.createFileManager();
	instance.createSourceManager(instance.getFileManager());

	// Create code completion consumer
	CodeCompleteOptions codeCompleteOptions;
	codeCompleteOptions.IncludeMacros = 0;
	codeCompleteOptions.IncludeCodePatterns = 0;
	codeCompleteOptions.IncludeGlobals = 1;
	codeCompleteOptions.IncludeBriefComments = 1;
	instance.setCodeCompletionConsumer(new CustomCodeCompleteConsumer(codeCompleteOptions));

	auto& preprocessorOptions = instance.getPreprocessorOpts();
	preprocessorOptions.clearRemappedFiles();
	if (buffer != nullptr) preprocessorOptions.addRemappedFile(filename, buffer);
	instance.createPreprocessor(TranslationUnitKind::TU_Complete);
	auto& preprocessor = instance.getPreprocessor();
	auto& builtinInfo = preprocessor.getBuiltinInfo();
	builtinInfo.InitializeTarget(instance.getTarget(), nullptr);
	auto& headerSearch = preprocessor.getHeaderSearchInfo();
	auto& headerSearchOpts = instance.getHeaderSearchOpts();
	std::vector<clang::DirectoryLookup> lookups;
	auto& fileManager = instance.getFileManager();
	for (const auto standardInclude : standardIncludes)
	{
		headerSearchOpts.UserEntries.emplace_back(standardInclude, frontend::IncludeDirGroup::System, false, false);
		lookups.emplace_back(fileManager.getDirectory(standardInclude), SrcMgr::CharacteristicKind::C_System, false);
	}
	headerSearch.SetSearchPaths(lookups, 0, 0, true);

	// Create frontend options
	auto& frontendOptions = instance.getFrontendOpts();
	frontendOptions.CodeCompletionAt.FileName = filename;
	frontendOptions.CodeCompletionAt.Line = row;
	frontendOptions.CodeCompletionAt.Column = column;
	FrontendInputFile input(filename, InputKind::IK_CXX);
	frontendOptions.Inputs.push_back(input);

	SyntaxOnlyAction action;
	python::list result;
	if (action.BeginSourceFile(instance, input))
	{
		action.Execute();
		action.EndSourceFile();
		auto consumer = static_cast<const CustomCodeCompleteConsumer*>(&instance.getCodeCompletionConsumer());
		result = consumer->getPythonResultList();
	}
	return result;
}
