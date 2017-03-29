#pragma once

#include "RegionClassifierVisitor.hpp"
#include <clang/AST/ASTConsumer.h>

namespace Clara
{

class RegionClassifierConsumer : public clang::ASTConsumer
{
  public:
    void HandleTranslationUnit(clang::ASTContext &ctx) override
    {
        // Traversing the translation unit decl via a RecursiveASTVisitor
        // will visit all nodes in the AST.
        mVisitor.TraverseDecl(ctx.getTranslationUnitDecl());
    }

    void moveResult(
        std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
            &regions)
    {
        mVisitor.moveResult(regions);
    }

  private:
    // A RecursiveASTVisitor implementation.
    RegionClassifierVisitor mVisitor;
};

} // namespace Clara
