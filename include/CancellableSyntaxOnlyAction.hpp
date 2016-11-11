#pragma once

#include <clang/Frontend/FrontendActions.h>

namespace Clara {

class CancellableASTConsumer;
	
class CancellableSyntaxOnlyAction : public clang::SyntaxOnlyAction
{
public:

	bool isCancelledAtomic() const noexcept;
	void setCancelAtomic(const bool);
	void Execute();

protected:

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &instance, 
		llvm::StringRef inFile) override;

private:

	std::atomic_bool mCancel;

	CancellableASTConsumer* mConsumer = nullptr;

};

} // namespace Clara