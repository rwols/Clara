#include "CancellableSyntaxOnlyAction.hpp"
#include "CancellableASTConsumer.hpp"
#include "CancelException.hpp"

namespace Clara {

std::unique_ptr<clang::ASTConsumer> CancellableSyntaxOnlyAction::CreateASTConsumer(
	clang::CompilerInstance &instance, 
	llvm::StringRef inFile)
{
	mConsumer = new CancellableASTConsumer(this);
	return std::unique_ptr<CancellableASTConsumer>(mConsumer);
}

bool CancellableSyntaxOnlyAction::isCancelledAtomic() const noexcept
{
	return mCancel;
}

void CancellableSyntaxOnlyAction::setCancelAtomic(const bool b)
{
	mCancel = b;
	if (mConsumer)
	{
		mConsumer->cancel.store(mCancel.load());
	}
}

void CancellableSyntaxOnlyAction::Execute()
{
	mCancel = false;
	if (mConsumer)
	{
		mConsumer->cancel.store(mCancel.load());
	}
	clang::SyntaxOnlyAction::Execute();
}

} // namespace Clara