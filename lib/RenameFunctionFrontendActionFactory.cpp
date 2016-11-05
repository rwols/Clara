#include "RenameFunctionFrontendActionFactory.hpp"
#include "RenameFunctionFrontendAction.hpp"

clang::FrontendAction* RenameFunctionFrontendActionFactory::create()
{
	auto action = new RenameFunctionFrontendAction();
	action->rewriter = rewriter;
	action->oldFullyQualifiedName = oldFullyQualifiedName;
	action->newFullyQualifiedName = newFullyQualifiedName;
	return action;
}