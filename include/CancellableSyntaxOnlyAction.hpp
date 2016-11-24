#pragma once

#include <clang/Frontend/FrontendActions.h>
#include <condition_variable>

namespace Clara {

class CancellableASTConsumer;
class Session;
	
class CancellableSyntaxOnlyAction : public clang::SyntaxOnlyAction
{
public:

	CancellableSyntaxOnlyAction(Session& owner);

	/**
	 * @brief      Blocks until the action is cancelled.
	 */
	void cancel();

protected:

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
		clang::CompilerInstance &instance, 
		llvm::StringRef inFile) override;

private:

	friend class CancellableASTConsumer;

	Session& mOwner;
	std::condition_variable mCancelVar;
	std::mutex mCancelMutex;
	bool mPleaseCancel = false;
	CancellableASTConsumer* mConsumer = nullptr;

};

} // namespace Clara