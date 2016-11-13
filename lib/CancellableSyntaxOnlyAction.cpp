#include "CancellableSyntaxOnlyAction.hpp"
#include "CancellableASTConsumer.hpp"

namespace Clara {

std::unique_ptr<clang::ASTConsumer> CancellableSyntaxOnlyAction::CreateASTConsumer(
	clang::CompilerInstance &instance, 
	llvm::StringRef inFile)
{
	return std::make_unique<CancellableASTConsumer>(*this);
}


void CancellableSyntaxOnlyAction::cancel()
{
	if (mConsumer == nullptr) return;
	std::unique_lock<std::mutex> lock(mCancelMutex);
	mPleaseCancel = true;
	mCancelVar.wait(lock, [this]{ return mConsumer == nullptr; });
}

} // namespace Clara