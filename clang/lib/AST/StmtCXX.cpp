//===--- StmtCXX.cpp - Classes for representing C++ statements ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the subclesses of Stmt class declared in StmtCXX.h
//
//===----------------------------------------------------------------------===//

#include "clang/AST/StmtCXX.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ExprCXX.h"

using namespace clang;

QualType CXXCatchStmt::getCaughtType() const {
  if (ExceptionDecl)
    return ExceptionDecl->getType();
  return QualType();
}

CXXTryStmt *CXXTryStmt::Create(const ASTContext &C, SourceLocation tryLoc,
                               CompoundStmt *tryBlock,
                               ArrayRef<Stmt *> handlers) {
  const size_t Size = totalSizeToAlloc<Stmt *>(handlers.size() + 1);
  void *Mem = C.Allocate(Size, alignof(CXXTryStmt));
  return new (Mem) CXXTryStmt(tryLoc, tryBlock, handlers);
}

CXXTryStmt *CXXTryStmt::Create(const ASTContext &C, EmptyShell Empty,
                               unsigned numHandlers) {
  const size_t Size = totalSizeToAlloc<Stmt *>(numHandlers + 1);
  void *Mem = C.Allocate(Size, alignof(CXXTryStmt));
  return new (Mem) CXXTryStmt(Empty, numHandlers);
}

CXXTryStmt::CXXTryStmt(SourceLocation tryLoc, CompoundStmt *tryBlock,
                       ArrayRef<Stmt *> handlers)
    : Stmt(CXXTryStmtClass), TryLoc(tryLoc), NumHandlers(handlers.size()) {
  Stmt **Stmts = getStmts();
  Stmts[0] = tryBlock;
  llvm::copy(handlers, Stmts + 1);
}

CXXForRangeStmt::CXXForRangeStmt(Stmt *Init, DeclStmt *Range,
                                 DeclStmt *BeginStmt, DeclStmt *EndStmt,
                                 Expr *Cond, Expr *Inc, DeclStmt *LoopVar,
                                 Stmt *Body, SourceLocation FL,
                                 SourceLocation CAL, SourceLocation CL,
                                 SourceLocation RPL)
    : Stmt(CXXForRangeStmtClass), ForLoc(FL), CoawaitLoc(CAL), ColonLoc(CL),
      RParenLoc(RPL) {
  SubExprs[INIT] = Init;
  SubExprs[RANGE] = Range;
  SubExprs[BEGINSTMT] = BeginStmt;
  SubExprs[ENDSTMT] = EndStmt;
  SubExprs[COND] = Cond;
  SubExprs[INC] = Inc;
  SubExprs[LOOPVAR] = LoopVar;
  SubExprs[BODY] = Body;
}

Expr *CXXForRangeStmt::getRangeInit() {
  DeclStmt *RangeStmt = getRangeStmt();
  VarDecl *RangeDecl = dyn_cast_or_null<VarDecl>(RangeStmt->getSingleDecl());
  assert(RangeDecl && "for-range should have a single var decl");
  return RangeDecl->getInit();
}

const Expr *CXXForRangeStmt::getRangeInit() const {
  return const_cast<CXXForRangeStmt *>(this)->getRangeInit();
}

VarDecl *CXXForRangeStmt::getLoopVariable() {
  Decl *LV = cast<DeclStmt>(getLoopVarStmt())->getSingleDecl();
  assert(LV && "No loop variable in CXXForRangeStmt");
  return cast<VarDecl>(LV);
}

const VarDecl *CXXForRangeStmt::getLoopVariable() const {
  return const_cast<CXXForRangeStmt *>(this)->getLoopVariable();
}

CoroutineBodyStmt *CoroutineBodyStmt::Create(
    const ASTContext &C, CoroutineBodyStmt::CtorArgs const &Args) {
  std::size_t Size = totalSizeToAlloc<Stmt *>(
      CoroutineBodyStmt::FirstParamMove + Args.ParamMoves.size());

  void *Mem = C.Allocate(Size, alignof(CoroutineBodyStmt));
  return new (Mem) CoroutineBodyStmt(Args);
}

CoroutineBodyStmt *CoroutineBodyStmt::Create(const ASTContext &C, EmptyShell,
                                             unsigned NumParams) {
  std::size_t Size = totalSizeToAlloc<Stmt *>(
      CoroutineBodyStmt::FirstParamMove + NumParams);

  void *Mem = C.Allocate(Size, alignof(CoroutineBodyStmt));
  auto *Result = new (Mem) CoroutineBodyStmt(CtorArgs());
  Result->NumParams = NumParams;
  auto *ParamBegin = Result->getStoredStmts() + SubStmt::FirstParamMove;
  std::uninitialized_fill(ParamBegin, ParamBegin + NumParams,
                          static_cast<Stmt *>(nullptr));
  return Result;
}

CoroutineBodyStmt::CoroutineBodyStmt(CoroutineBodyStmt::CtorArgs const &Args)
    : Stmt(CoroutineBodyStmtClass), NumParams(Args.ParamMoves.size()) {
  Stmt **SubStmts = getStoredStmts();
  SubStmts[CoroutineBodyStmt::Body] = Args.Body;
  SubStmts[CoroutineBodyStmt::Promise] = Args.Promise;
  SubStmts[CoroutineBodyStmt::InitSuspend] = Args.InitialSuspend;
  SubStmts[CoroutineBodyStmt::FinalSuspend] = Args.FinalSuspend;
  SubStmts[CoroutineBodyStmt::OnException] = Args.OnException;
  SubStmts[CoroutineBodyStmt::OnFallthrough] = Args.OnFallthrough;
  SubStmts[CoroutineBodyStmt::Allocate] = Args.Allocate;
  SubStmts[CoroutineBodyStmt::Deallocate] = Args.Deallocate;
  SubStmts[CoroutineBodyStmt::ResultDecl] = Args.ResultDecl;
  SubStmts[CoroutineBodyStmt::ReturnValue] = Args.ReturnValue;
  SubStmts[CoroutineBodyStmt::ReturnStmt] = Args.ReturnStmt;
  SubStmts[CoroutineBodyStmt::ReturnStmtOnAllocFailure] =
      Args.ReturnStmtOnAllocFailure;
  llvm::copy(Args.ParamMoves, const_cast<Stmt **>(getParamMoves().data()));
}

CXXExpansionStmt::CXXExpansionStmt(
    StmtClass SC, Stmt *Init, DeclStmt *ExpansionVar, Expr *SizeExpr,
    SourceLocation TemplateKWLoc, SourceLocation ForLoc,
    SourceLocation LParenLoc, SourceLocation ColonLoc, SourceLocation RParenLoc,
    Expr *TParamRef)
    : Stmt(SC),
      SubStmts {Init, TParamRef, ExpansionVar, SizeExpr, nullptr},
      Expansions(nullptr), TemplateKWLoc(TemplateKWLoc), ForLoc(ForLoc),
      LParenLoc(LParenLoc), ColonLoc(ColonLoc), RParenLoc(RParenLoc) { }

VarDecl *CXXExpansionStmt::getExpansionVariable() {
  Decl *EV = cast<DeclStmt>(getExpansionVarStmt())->getSingleDecl();
  assert(EV && "No expansion variable in CXXExpansionStmt");
  return cast<VarDecl>(EV);
}

bool CXXExpansionStmt::hasDependentSize() const {
  switch (getStmtClass()) {
  case CXXIndeterminateExpansionStmtClass:
    return true;
  case CXXIterableExpansionStmtClass:
    return cast<CXXIterableExpansionStmt>(this)->hasDependentSize();
  case CXXDestructurableExpansionStmtClass:
    return cast<CXXDestructurableExpansionStmt>(this)->hasDependentSize();
  case CXXInitListExpansionStmtClass:
    return cast<CXXInitListExpansionStmt>(this)->hasDependentSize();
  }
  llvm_unreachable("unknown expansion statement kind");
}

unsigned CXXExpansionStmt::getNumInstantiations() const {
  assert(!hasDependentSize());

  switch (getStmtClass()) {
  case CXXIterableExpansionStmtClass:
    return cast<CXXIterableExpansionStmt>(this)->getNumInstantiations();
  case CXXDestructurableExpansionStmtClass:
    return cast<CXXDestructurableExpansionStmt>(this)->getNumInstantiations();
  case CXXInitListExpansionStmtClass:
    return cast<CXXInitListExpansionStmt>(this)->getNumInstantiations();
  }
  llvm_unreachable("unknown expansion statement kind");
}

CXXIndeterminateExpansionStmt *CXXIndeterminateExpansionStmt::Create(
    const ASTContext &C, Stmt *Init, DeclStmt *ExpansionVar,
    SourceLocation TemplateKWLoc, SourceLocation ForLoc,
    SourceLocation LParenLoc, SourceLocation ColonLoc, SourceLocation RParenLoc,
    Expr *TParamRef) {
  return new (C) CXXIndeterminateExpansionStmt(Init, ExpansionVar,
                                               TemplateKWLoc, ForLoc, LParenLoc,
                                               ColonLoc, RParenLoc, TParamRef);
}

CXXIndeterminateExpansionStmt *CXXIndeterminateExpansionStmt::Create(
    const ASTContext &C, EmptyShell Empty) {
  return new (C) CXXIndeterminateExpansionStmt(Empty);
}

CXXIterableExpansionStmt *CXXIterableExpansionStmt::Create(
    const ASTContext &C, Stmt *Init, DeclStmt *ExpansionVar, Expr *SizeExpr,
    unsigned NumInstantiations, SourceLocation TemplateKWLoc,
    SourceLocation ForLoc, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation RParenLoc, Expr *TParamRef) {
  return new (C) CXXIterableExpansionStmt(Init, ExpansionVar, SizeExpr,
                                          NumInstantiations, TemplateKWLoc,
                                          ForLoc, LParenLoc, ColonLoc,
                                          RParenLoc, TParamRef);
}

CXXIterableExpansionStmt *CXXIterableExpansionStmt::Create(const ASTContext &C,
                                                           EmptyShell Empty) {
  return new (C) CXXIterableExpansionStmt(Empty);
}

bool CXXIterableExpansionStmt::hasDependentSize() const {
  return cast<Expr>(getSizeExpr())->isValueDependent();
}

unsigned CXXIterableExpansionStmt::getNumInstantiations() const {
  return NumInstantiations;
}

CXXDestructurableExpansionStmt *CXXDestructurableExpansionStmt::Create(
    const ASTContext &C, Stmt *Init, DeclStmt *ExpansionVar,
    SourceLocation TemplateKWLoc, SourceLocation ForLoc,
    SourceLocation LParenLoc, SourceLocation ColonLoc, SourceLocation RParenLoc,
    Expr *TParamRef) {
  return new (C) CXXDestructurableExpansionStmt(Init, ExpansionVar,
                                                TemplateKWLoc, ForLoc,
                                                LParenLoc, ColonLoc, RParenLoc,
                                                TParamRef);
}

CXXDestructurableExpansionStmt *CXXDestructurableExpansionStmt::Create(
    const ASTContext &C, EmptyShell Empty) {
  return new (C) CXXDestructurableExpansionStmt(Empty);
}

bool CXXDestructurableExpansionStmt::hasDependentSize() const {
  return false;
}

unsigned CXXDestructurableExpansionStmt::getNumInstantiations() const {
  const VarDecl *VD = getExpansionVariable();
  auto *Selector = cast<CXXDestructurableExpansionSelectExpr>(VD->getInit());

  return Selector->getDecompositionDecl()->bindings().size();
}

CXXInitListExpansionStmt *CXXInitListExpansionStmt::Create(
    const ASTContext &C, Stmt *Init, DeclStmt *ExpansionVar,
    SourceLocation TemplateKWLoc, SourceLocation ForLoc,
    SourceLocation LParenLoc, SourceLocation ColonLoc, SourceLocation RParenLoc,
    Expr *TParamRef) {
  return new (C) CXXInitListExpansionStmt(Init, ExpansionVar, TemplateKWLoc,
                                          ForLoc, LParenLoc, ColonLoc,
                                          RParenLoc, TParamRef);
}

CXXInitListExpansionStmt *CXXInitListExpansionStmt::Create(const ASTContext &C,
                                                           EmptyShell Empty) {
  return new (C) CXXInitListExpansionStmt(Empty);
}

bool CXXInitListExpansionStmt::hasDependentSize() const {
  const auto *Init = cast<CXXExpansionInitListSelectExpr>(
      getExpansionVariable()->getInit());
  return cast<CXXExpansionInitListExpr>(Init->getRangeExpr())->containsPack();
}

unsigned CXXInitListExpansionStmt::getNumInstantiations() const {
  const auto *Init = cast<CXXExpansionInitListSelectExpr>(
      getExpansionVariable()->getInit());
  return cast<CXXExpansionInitListExpr>(Init->getRangeExpr())
      ->getSubExprs().size();
}
