//===--- SemaExpand.cpp - Semantic Analysis for Expansion Statements-------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements semantic analysis for expansion statements.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/Basic/DiagnosticSema.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/Template.h"  // for expansion statements

using namespace clang;
using namespace sema;

namespace {

VarDecl *ExtractVarDecl(Stmt *S) {
  if (auto *DS = dyn_cast_or_null<DeclStmt>(S); S)
    if (Decl *D = DS->getSingleDecl(); D && !D->isInvalidDecl())
      return dyn_cast<VarDecl>(D);
  return nullptr;
}

unsigned ExtractParmVarDeclDepth(Expr *E) {
  if (auto *DRE = cast<DeclRefExpr>(E))
    if (auto *PVD = cast<NonTypeTemplateParmDecl>(DRE->getDecl()))
      return PVD->getDepth();
  return 0;
}

// Returns how many layers of templates the current scope is nested within.
unsigned ComputeTemplateEmbeddingDepth(Scope *CurScope) {
  int Depth = 0;
  while ((CurScope = CurScope->getParent())) {
    if (CurScope->isTemplateParamScope())
      ++Depth;
  }
  return Depth;
}

}  // close anonymous namespace

StmtResult Sema::ActOnCXXExpansionStmt(Scope *S, SourceLocation TemplateKWLoc,
                                       SourceLocation ForLoc,
                                       SourceLocation LParenLoc, Stmt *Init,
                                       Stmt *ExpansionVarStmt,
                                       SourceLocation ColonLoc, Expr *Range,
                                       SourceLocation RParenLoc,
                                       BuildForRangeKind Kind) {
  // Compute how many layers of template parameters wrap this statement.
  unsigned TemplateDepth = ComputeTemplateEmbeddingDepth(S);

  // Create a template parameter '__N'.
  IdentifierInfo *ParmName = &Context.Idents.get("__N");
  QualType ParmTy = Context.getSizeType();
  TypeSourceInfo *ParmTI = Context.getTrivialTypeSourceInfo(ParmTy, ColonLoc);

  NonTypeTemplateParmDecl *TParm =
        NonTypeTemplateParmDecl::Create(Context,
                                        Context.getTranslationUnitDecl(),
                                        ColonLoc, ColonLoc, TemplateDepth,
                                        /*Position=*/0, ParmName, ParmTy, false,
                                        ParmTI);

  // Build a 'DeclRefExpr' designating the template parameter '__N'.
  ExprResult ER = BuildDeclRefExpr(TParm, Context.getSizeType(), VK_PRValue,
                                   ColonLoc);
  if (ER.isInvalid())
    return StmtError();
  Expr *TParmRef = ER.get();

  // Build an expansion statement depending on what kind of 'Range' we have.
  if (auto *EILE = dyn_cast<CXXExpansionInitListExpr>(Range))
    return ActOnCXXInitListExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc, Init,
                                         ExpansionVarStmt, ColonLoc, EILE,
                                         RParenLoc, TParmRef, Kind);

  llvm_unreachable("unknown expansion statement kind");
}

StmtResult Sema::ActOnCXXInitListExpansionStmt(SourceLocation TemplateKWLoc,
                                               SourceLocation ForLoc,
                                               SourceLocation LParenLoc,
                                               Stmt *Init,
                                               Stmt *ExpansionVarStmt,
                                               SourceLocation ColonLoc,
                                               CXXExpansionInitListExpr *Range,
                                               SourceLocation RParenLoc,
                                               Expr *TParmRef,
                                               BuildForRangeKind Kind) {
  // Extract the declaration of the expansion variable.
  VarDecl *ExpansionVar = ExtractVarDecl(ExpansionVarStmt);
  if (!ExpansionVar || Kind == BFRK_Check)
    return StmtError();

  // Build accessor for getting the __N'th Expr from the expression-init-list.
  ExprResult Accessor = ActOnCXXExpansionSelectExpr(Range, TParmRef);
  if (Accessor.isInvalid())
    return StmtError();

  // Attach the accessor as the initializer for the expansion variable.
  AddInitializerToDecl(ExpansionVar, Accessor.get(), /*DirectInit=*/false);
  if (ExpansionVar->isInvalidDecl())
    return StmtError();

  unsigned TemplateDepth = ExtractParmVarDeclDepth(TParmRef);
  return BuildCXXInitListExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc, Init,
                                       ExpansionVarStmt, ColonLoc, Range,
                                       RParenLoc, TemplateDepth, Kind);
}

ExprResult Sema::ActOnCXXExpansionInitList(SourceLocation LBraceLoc,
                                           MultiExprArg SubExprs,
                                           SourceLocation RBraceLoc) {
  return BuildCXXExpansionInitList(LBraceLoc, SubExprs, RBraceLoc);
}

ExprResult Sema::ActOnCXXExpansionSelectExpr(Expr *Range, Expr *Idx) {
  if (auto *EILE = dyn_cast<CXXExpansionInitListExpr>(Range))
    return BuildCXXExpansionSelectExpr(EILE, Idx);

  llvm_unreachable("unknown expansion statement kind");
}

StmtResult Sema::BuildCXXInitListExpansionStmt(SourceLocation TemplateKWLoc,
                                               SourceLocation ForLoc,
                                               SourceLocation LParenLoc,
                                               Stmt *Init,
                                               Stmt *ExpansionVarStmt,
                                               SourceLocation ColonLoc,
                                               CXXExpansionInitListExpr *Range,
                                               SourceLocation RParenLoc,
                                               unsigned TemplateDepth,
                                               BuildForRangeKind Kind) {
  return CXXInitListExpansionStmt::Create(Context, Init,
                                          cast<DeclStmt>(ExpansionVarStmt),
                                          Range, Range->getSubExprs().size(),
                                          TemplateKWLoc, ForLoc, LParenLoc,
                                          ColonLoc, RParenLoc, TemplateDepth);
}

ExprResult Sema::BuildCXXExpansionInitList(SourceLocation LBraceLoc,
                                           MultiExprArg SubExprs,
                                           SourceLocation RBraceLoc) {
  Expr **SubExprList = new (Context) Expr *[SubExprs.size()];
  std::memcpy(SubExprList, SubExprs.data(), SubExprs.size() * sizeof(Expr *));

  return CXXExpansionInitListExpr::Create(Context, SubExprList,
                                          SubExprs.size(), LBraceLoc,
                                          RBraceLoc);
}

ExprResult Sema::BuildCXXExpansionSelectExpr(CXXExpansionInitListExpr *Range,
                                             Expr *Idx) {
  // Use 'CXXExpansionSelectExpr' as a placeholder before tree transform.
  if (Range->containsPack() || Idx->isValueDependent()) {
    return CXXExpansionSelectExpr::Create(Context, Range, Idx);
  }
  ArrayRef<Expr *> SubExprs = Range->getSubExprs();

  // Evaluate the index.
  Expr::EvalResult ER;
  if (!Idx->EvaluateAsInt(ER, Context))
    return ExprError();  // TODO: Diagnostic.
  size_t I = ER.Val.getInt().getZExtValue();
  assert(I < SubExprs.size());

  return SubExprs[I];
}

StmtResult Sema::FinishCXXExpansionStmt(Stmt *Heading, Stmt *Body) {
  if (!Heading || !Body)
    return StmtError();

  CXXExpansionStmt *Expansion = cast<CXXExpansionStmt>(Heading);
  Expansion->setBody(Body);

  // Canonical location for instantiations.
  SourceLocation Loc = Expansion->getColonLoc();

  if (Expansion->hasDependentSize())
    return Expansion;

  // Return an empty statement if the range is empty.
  if (Expansion->getNumInstantiations() == 0)
     return Expansion;

  // Create a compound statement binding loop and body.
  Stmt *VarAndBody[] = {Expansion->getExpansionVarStmt(), Body};
  Stmt *CombinedBody = CompoundStmt::Create(Context, VarAndBody,
                                            FPOptionsOverride(),
                                            Expansion->getBeginLoc(),
                                            Expansion->getEndLoc());

  // Expand the body for each instantiation.
  Stmt **Instantiations =
        new (Context) Stmt *[Expansion->getNumInstantiations()];
  for (size_t I = 0; I < Expansion->getNumInstantiations(); ++I) {
    IntegerLiteral *Idx = IntegerLiteral::Create(Context,
                                                 llvm::APSInt::getUnsigned(I),
                                                 Context.getSizeType(), Loc);
    TemplateArgument TArgs[] = {
        { Context, llvm::APSInt(Idx->getValue(), true), Idx->getType() }
    };
    MultiLevelTemplateArgumentList MTArgList(nullptr, TArgs, true);
    MTArgList.addOuterRetainedLevels(Expansion->getTemplateDepth());
    LocalInstantiationScope LIScope(*this, /*CombineWithOuterScope=*/true);
    InstantiatingTemplate Inst(*this, Body->getBeginLoc(), Expansion, TArgs,
                               Body->getSourceRange());

    StmtResult Instantiation = SubstStmt(CombinedBody, MTArgList);
    if (Instantiation.isInvalid())
      return StmtError();
    Instantiations[I] = Instantiation.get();
  }

  Expansion->setInstantiations(Instantiations);
  return Expansion;
}
