#include "CancellableASTConsumer.hpp"
#include <clang/AST/DeclGroup.h>

namespace Clara {

CancellableASTConsumer::CancellableASTConsumer(CancellableSyntaxOnlyAction* creator)
: mCreator(creator), cancel(false)
{

}

bool CancellableASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef declGroup)
{
	return cancel ? false : ASTConsumer::HandleTopLevelDecl(declGroup); 
}

} // namespace Clara