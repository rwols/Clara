#include "RegionClassifierVisitor.hpp"
#include <clang/Frontend/ASTUnit.h>

using namespace Clara;

RegionClassifierVisitor::RegionClassifierVisitor(const clang::ASTUnit *unit,
                                                 pybind11::object log)
    : mUnit(const_cast<clang::ASTUnit *const>(unit)), mLog(log)
{
}

void RegionClassifierVisitor::addRegion(clang::Decl *decl, const char *key)
{
    const auto &sourceMgr = mUnit->getSourceManager();
    auto start = decl->getLocStart();
    auto end = decl->getLocEnd();
    start = sourceMgr.getSpellingLoc(start);
    end = sourceMgr.getSpellingLoc(end);
    const auto startOffset = sourceMgr.getFileOffset(start);
    const auto endOffset = sourceMgr.getFileOffset(end);
    std::string msg;
    llvm::raw_string_ostream ss(msg);
    decl->dump(ss);
    ss.flush();
    {
        pybind11::gil_scoped_acquire lock;
        mLog(std::move(msg));
    }
    mRegions[key].emplace_back(startOffset, endOffset);
}

bool RegionClassifierVisitor::VisitRecordDecl(clang::RecordDecl *decl)
{
    if (!mUnit->isInMainFileID(decl->getLocStart()))
    {
        return true;
    }
    if (decl->isClass())
    {
        addRegion(decl, "entity.name.class");
    }
    else if (decl->isStruct())
    {
        addRegion(decl, "entity.name.struct");
    }
    else if (decl->isNamespace())
    {
        addRegion(decl, "entity.name.namespace");
    }
    else if (decl->isEnum())
    {
        addRegion(decl, "entity.name.enum");
    }
    return true;
}

bool RegionClassifierVisitor::VisitValueDecl(clang::ValueDecl *decl)
{
    if (!mUnit->isInMainFileID(decl->getLocStart()))
    {
        return true;
    }
    {
        pybind11::gil_scoped_acquire lock;
        mLog("visiting ValueDecl: " + decl->getNameAsString() + ", of type " +
             decl->getType().getAsString());
    }
    const auto type = decl->getType().getTypePtrOrNull();
    if (type == nullptr) return true;
    const auto tag = type->getAsTagDecl();
    if (tag == nullptr) return true;
    if (tag->isClass())
    {
        addRegion(decl, "entity.name.class");
    }
    else if (tag->isStruct())
    {
        addRegion(decl, "entity.name.struct");
    }
    else if (tag->isNamespace())
    {
        addRegion(decl, "entity.name.namespace");
    }
    else if (tag->isEnum())
    {
        addRegion(decl, "entity.name.enum");
    }
    return true;
}

void RegionClassifierVisitor::moveResult(
    std::map<std::string, std::vector<std::pair<std::size_t, std::size_t>>>
        &regions)
{
    regions = std::move(mRegions);
}
