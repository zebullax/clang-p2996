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
#include "clang/Sema/Template.h"


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
  if (!Range)
    return StmtError();

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
  else
    return ActOnCXXDestructurableExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc,
                                               Init, ExpansionVarStmt, ColonLoc,
                                               Range, RParenLoc, TParmRef,
                                               Kind);
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
  ExprResult Accessor = ActOnCXXExpansionInitListSelectExpr(Range, TParmRef);
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

StmtResult Sema::ActOnCXXDestructurableExpansionStmt(
    SourceLocation TemplateKWLoc, SourceLocation ForLoc,
    SourceLocation LParenLoc, Stmt *Init, Stmt *ExpansionVarStmt,
    SourceLocation ColonLoc, Expr *Range, SourceLocation RParenLoc,
    Expr *TParmRef, BuildForRangeKind Kind) {
  // Extract the declaration of the expansion variable.
  VarDecl *ExpansionVar = ExtractVarDecl(ExpansionVarStmt);
  if (!ExpansionVar || Kind == BFRK_Check)
    return StmtError();

  // Build accessor for getting the expression naming the __N'th subobject.
  bool Constexpr = ExpansionVar->isConstexpr();
  ExprResult Accessor = ActOnCXXDestructurableExpansionSelectExpr(Range,
                                                                  TParmRef,
                                                                  Constexpr);
  if (Accessor.isInvalid())
    return StmtError();

  // Attach the accessor as the initializerfor the expansion variable.
  AddInitializerToDecl(ExpansionVar, Accessor.get(), /*DirectInit=*/false);
  if (ExpansionVar->isInvalidDecl())
    return StmtError();

  unsigned TemplateDepth = ExtractParmVarDeclDepth(TParmRef);
  return BuildCXXDestructurableExpansionStmt(TemplateKWLoc, ForLoc, LParenLoc,
                                             Init, ExpansionVarStmt, ColonLoc,
                                             Range, RParenLoc, TemplateDepth,
                                             Kind);
}

ExprResult Sema::ActOnCXXExpansionInitList(SourceLocation LBraceLoc,
                                           MultiExprArg SubExprs,
                                           SourceLocation RBraceLoc) {
  return BuildCXXExpansionInitList(LBraceLoc, SubExprs, RBraceLoc);
}

ExprResult
Sema::ActOnCXXExpansionInitListSelectExpr(CXXExpansionInitListExpr *Range,
                                          Expr *Idx) {
  return BuildCXXExpansionInitListSelectExpr(Range, Idx);
}

ExprResult Sema::ActOnCXXDestructurableExpansionSelectExpr(Expr *Range,
                                                           Expr *Idx,
                                                           bool Constexpr) {
  return BuildCXXDestructurableExpansionSelectExpr(Range, nullptr, Idx,
                                                   Constexpr);
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

StmtResult
Sema::BuildCXXDestructurableExpansionStmt(SourceLocation TemplateKWLoc,
                                          SourceLocation ForLoc,
                                          SourceLocation LParenLoc,
                                          Stmt *Init, Stmt *ExpansionVarStmt,
                                          SourceLocation ColonLoc, Expr *Range,
                                          SourceLocation RParenLoc,
                                          unsigned TemplateDepth,
                                          BuildForRangeKind Kind) {
  VarDecl *VD = ExtractVarDecl(ExpansionVarStmt);
  if (!VD || Kind == BFRK_Check)
    return StmtError();

  unsigned NumExpansions = 0;
  if (auto *SE = dyn_cast<CXXDestructurableExpansionSelectExpr>(VD->getInit());
      SE && SE->getDecompositionDecl())
    NumExpansions = SE->getDecompositionDecl()->bindings().size();

  return CXXDestructurableExpansionStmt::Create(
          Context, Init, cast<DeclStmt>(ExpansionVarStmt), Range, NumExpansions,
          TemplateKWLoc, ForLoc, LParenLoc, ColonLoc, RParenLoc, TemplateDepth);
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

ExprResult
Sema::BuildCXXExpansionInitListSelectExpr(CXXExpansionInitListExpr *Range,
                                          Expr *Idx) {
  // Use 'CXXExpansionInitListSelectExpr' as a placeholder until tree transform.
  if (Range->containsPack() || Idx->isValueDependent())
    return CXXExpansionInitListSelectExpr::Create(Context, Range, Idx);
  ArrayRef<Expr *> SubExprs = Range->getSubExprs();

  // Evaluate the index.
  Expr::EvalResult ER;
  if (!Idx->EvaluateAsInt(ER, Context))
    return ExprError();  // TODO: Diagnostic.
  size_t I = ER.Val.getInt().getZExtValue();
  assert(I < SubExprs.size());

  return SubExprs[I];
}

ExprResult
Sema::BuildCXXDestructurableExpansionSelectExpr(Expr *Range,
                                                DecompositionDecl *DD,
                                                Expr *Idx, bool Constexpr) {
  assert (!isa<CXXExpansionInitListExpr>(Range) &&
          "expansion-init-list should never have structured bindings");

  if (!DD && !Range->isTypeDependent() && !Range->isValueDependent()) {
    unsigned Arity;
    if (!ComputeDecompositionExpansionArity(Range, Arity))
      return ExprError();

    SmallVector<BindingDecl *, 4> Bindings;
    for (size_t k = 0; k < Arity; ++k)
      Bindings.push_back(BindingDecl::Create(Context, CurContext,
                                             Range->getBeginLoc(),
                                             /*IdentifierInfo=*/nullptr));

    TypeSourceInfo *TSI = Context.getTrivialTypeSourceInfo(Range->getType());
    DD = DecompositionDecl::Create(Context, CurContext, Range->getBeginLoc(),
                                   Range->getBeginLoc(), Range->getType(), TSI,
                                   SC_Auto, Bindings);
    if (Constexpr)
      DD->setConstexpr(true);

    AddInitializerToDecl(DD, Range, false);
  }

  if (!DD || Idx->isValueDependent())
    return CXXDestructurableExpansionSelectExpr::Create(Context, Range, DD, Idx,
                                                        Constexpr);

  Expr::EvalResult ER;
  if (!Idx->EvaluateAsInt(ER, Context))
    return ExprError();  // TODO: Diagnostic.
  size_t I = ER.Val.getInt().getZExtValue();
  assert(I < DD->bindings().size());

  return DD->bindings()[I]->getBinding();
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
