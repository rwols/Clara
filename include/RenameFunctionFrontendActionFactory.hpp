#pragma once

#include <clang/Tooling/Tooling.h>
#include <clang/Rewrite/Core/Rewriter.h>

class RenameFunctionFrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
	
	std::string newFullyQualifiedName;
	std::string oldFullyQualifiedName;

	clang::Rewriter* rewriter = nullptr;

	virtual ~RenameFunctionFrontendActionFactory() noexcept = default;
	clang::FrontendAction* create() override;
};