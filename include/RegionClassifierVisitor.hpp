#pragma once

#include "PyBind11.hpp"
#include <clang/AST/RecursiveASTVisitor.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace clang
{
class ASTUnit;
}

namespace Clara
{

class RegionClassifierVisitor
    : public clang::RecursiveASTVisitor<RegionClassifierVisitor>
{
  public:
    RegionClassifierVisitor(const clang::ASTUnit *const unit,
                            pybind11::object log);

    bool VisitRecordDecl(clang::RecordDecl *);

    bool VisitValueDecl(clang::ValueDecl *);

    void moveResult(
        std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
            &regions);

  private:
    void addRegion(clang::Decl *decl, const char *key);

    clang::ASTUnit *const mUnit;
    std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
        mRegions;

    pybind11::object mLog;
};

} // namespace Clara
