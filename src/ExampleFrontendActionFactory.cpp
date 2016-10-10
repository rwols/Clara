#include "ExampleFrontendActionFactory.hpp"
#include "ExampleFrontendAction.hpp"

ExampleFrontendActionFactory::ExampleFrontendActionFactory(clang::Rewriter* rewriter)
: mRewriter(rewriter)
{

}

clang::FrontendAction* ExampleFrontendActionFactory::create()
{
	auto action = new ExampleFrontendAction();
	action->setRewriter(mRewriter);
	return action;
}