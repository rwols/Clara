#pragma once

#include <clang/AST/ASTConsumer.h>
#include <atomic>

namespace Clara {

class CancellableSyntaxOnlyAction;

class CancellableASTConsumer : public clang::ASTConsumer
{
public:

	CancellableASTConsumer(CancellableSyntaxOnlyAction& mCreator);
	~CancellableASTConsumer() noexcept;

	bool HandleTopLevelDecl(clang::DeclGroupRef D) override;

private:

	CancellableSyntaxOnlyAction& mCreator;

};

} // namespace Clara