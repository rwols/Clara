#pragma once

#include <clang/Tooling/Tooling.h>
#include <clang/Rewrite/Core/Rewriter.h>

class ExampleFrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
	ExampleFrontendActionFactory(clang::Rewriter* rewriter);
	virtual ~ExampleFrontendActionFactory() noexcept = default;
	clang::FrontendAction* create() override;
private:
	clang::Rewriter* mRewriter = nullptr;
};