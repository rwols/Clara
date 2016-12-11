#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/PCHContainerOperations.h>

#define MAKE_IT_CRASH

const char* CMD_LINE_ARGS[] =
{
	"clang++",
	"/Users/rwols/dev/llvm/tools/clang/tools/Clara/bin/crash.cpp",
	"-I/Users/rwols/dev/llvm/tools/clang/include",
	"-std=c++14",
	"-isystem",
	"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1",
	"-isystem",
	"/usr/local/include",
	"-isystem",
	"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/8.0.0/include",
	"-isystem",
	"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include",
	"-isystem",
	"/usr/include"
};

const char* RESOURCE_PATH = "/usr/local/lib/clang/4.0.0";

template <class T, class ...Args>
clang::IntrusiveRefCntPtr<T> makeIntrusive(Args&&... args)
{
	return new T(std::forward<Args>(args)...);
}

int main()
{
	#ifdef MAKE_IT_CRASH
	const int precompilePreamble = 1;
	#else
	const int precompilePreamble = 0;
	#endif

	using namespace clang;
	auto pchOps = std::make_shared<PCHContainerOperations>();
	auto diagIDs = makeIntrusive<DiagnosticIDs>();
	auto diagOpts = makeIntrusive<DiagnosticOptions>();
	TextDiagnosticPrinter consumer(llvm::outs(), diagOpts.get());
	auto diags = makeIntrusive<DiagnosticsEngine>(diagIDs.get(), diagOpts.get(),
		&consumer, false);
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-variable"
	auto unit = ASTUnit::LoadFromCommandLine(std::begin(CMD_LINE_ARGS),
		std::end(CMD_LINE_ARGS), pchOps, diags, RESOURCE_PATH,
		/*OnlyLocalDecls=*/false, /*CaptureDiagnostics=*/false,
		/*RemappedFiles=*/None, /*RemappedFilesKeepOriginalName=*/true,
		/*PrecompilePreambleAfterNParses=*/precompilePreamble);
	#pragma clang diagnostic pop
	return 0;
}