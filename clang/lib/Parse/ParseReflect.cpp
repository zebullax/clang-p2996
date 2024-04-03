//===--- ParseReflect.cpp - C++2c Reflection Parsing (P2996) --------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements parsing for reflection facilities.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/EnterExpressionEvaluationContext.h"
using namespace clang;

ExprResult Parser::ParseCXXReflectExpression() {
  assert(Tok.is(tok::caret) && "expected '^'");
  SourceLocation OpLoc = ConsumeToken();

  // Handle case of '^[: ... :]' specially.
  if (Tok.is(tok::l_splice)) {
    // Try to parse the splice as a part of a nested-name-specifier.
    CXXScopeSpec SS;
    if (ParseOptionalCXXScopeSpecifier(SS, /*ObjectType=*/nullptr,
                                       /*ObjectHasErrors=*/false,
                                       /*EnteringContext=*/false))
      return ExprError();

    // If 'SS' is empty but parsing did not fail, we should have an
    // 'annot_splice'. This is a lone splice token without a leading qualifier.
    if (SS.isEmpty()) {
      assert(Tok.is(tok::annot_splice));

      ExprResult ER = getExprAnnotation(Tok);
      assert(!ER.isInvalid());
      ConsumeAnnotationToken();

      return Actions.ActOnCXXReflectExpr(
            OpLoc, cast<CXXIndeterminateSpliceExpr>(ER.get()));
    }

    // Otherwise, insert 'SS' back into the token stream as an 'annot_scope',
    // and continue to parse as usual.
    AnnotateScopeToken(SS, true);
  }

  // Try parsing as a template-template argument.
  // This is probably "close enough" to "unparameterized template ID".
  {
    TentativeParsingAction TentativeAction(*this);
    ParsedTemplateArgument Template;
    {
      EnterExpressionEvaluationContext EvalContext(
          Actions, Sema::ExpressionEvaluationContext::ReflectionContext);
      Template = ParseTemplateReflectOperand();
    }
    if (!Template.isInvalid()) {
      TentativeAction.Commit();
      return Actions.ActOnCXXReflectExpr(OpLoc, Template);
    }
    TentativeAction.Revert();
  }

  // Try parsing as a namespace name.
  {
    TentativeParsingAction TentativeAction(*this);
    Decl *NSDecl;
    {
      EnterExpressionEvaluationContext EvalContext(
          Actions, Sema::ExpressionEvaluationContext::ReflectionContext);
      CXXScopeSpec SS;
      SourceLocation IdLoc;
      NSDecl = ParseNamespaceName(SS, IdLoc);
      if (NSDecl) {
        TentativeAction.Commit();
        return Actions.ActOnCXXReflectExpr(OpLoc, IdLoc, NSDecl);
      }
    }
    TentativeAction.Revert();
  }

  // Try parsing as type-id.
  if (isCXXTypeId(TypeIdAsReflectionOperand)) {
    TypeResult TR;
    {
      EnterExpressionEvaluationContext EvalContext(
          Actions, Sema::ExpressionEvaluationContext::ReflectionContext);
      TR = ParseTypeName(nullptr, DeclaratorContext::ReflectOperator);
    }
    if (TR.isInvalid()) {
      return ExprError();
    }
    return Actions.ActOnCXXReflectExpr(OpLoc, TR);
  }

  // Otherwise, parse as an expression.
  ExprResult E;
  {
    EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ReflectionContext);
    E = ParseCastExpression(AnyCastExpr, true);
  }
  if (E.isInvalid() || !E.get())
    return ExprError();

  return Actions.ActOnCXXReflectExpr(OpLoc, E.get());
}

ExprResult Parser::ParseCXXMetafunctionExpression() {
  assert(Tok.is(tok::kw___metafunction) && "expected '___metafunction'");
  SourceLocation KwLoc = ConsumeToken();

  // Balance any number of arguments in parens.
  BalancedDelimiterTracker Parens(*this, tok::l_paren);
  if (Parens.expectAndConsume())
    return ExprError();

  SmallVector<Expr *, 2> Args;
  do {
    ExprResult Expr = ParseConstantExpression();
    if (Expr.isInvalid()) {
      Parens.skipToEnd();
      return ExprError();
    }
    Args.push_back(Expr.get());
  } while (TryConsumeToken(tok::comma));

  if (Parens.consumeClose())
    return ExprError();

  SourceLocation LPLoc = Parens.getOpenLocation();
  SourceLocation RPLoc = Parens.getCloseLocation();
  return Actions.ActOnCXXMetafunction(KwLoc, LPLoc, Args, RPLoc);
}

bool Parser::ParseCXXIndeterminateSplice() {
  assert(Tok.is(tok::l_splice) && "expected '[:'");

  BalancedDelimiterTracker SpliceTokens(*this, tok::l_splice);
  if (SpliceTokens.expectAndConsume())
    return true;

  ExprResult ER;
  {
    EnterExpressionEvaluationContext EvalContext(
        Actions, Sema::ExpressionEvaluationContext::ConstantEvaluated);
    ER = ParseConstantExpression();
  }
  if (ER.isInvalid()) {
    SpliceTokens.skipToEnd();
    return true;
  }
  Expr *Operand = ER.get();

  Token end = Tok;
  if (SpliceTokens.consumeClose())
    return true;

  SourceLocation LSplice = SpliceTokens.getOpenLocation();
  SourceLocation RSplice = SpliceTokens.getCloseLocation();

  ER = Actions.ActOnCXXIndeterminateSpliceExpr(LSplice, Operand, RSplice);
  if (ER.isInvalid() || ER.get()->containsErrors())
    return true;
  Expr *SpliceExpr = ER.get();

  UnconsumeToken(end);
  Tok.setKind(tok::annot_splice);
  setExprAnnotation(Tok, SpliceExpr);
  Tok.setLocation(LSplice);
  Tok.setAnnotationEndLoc(RSplice);
  PP.AnnotateCachedTokens(Tok);

  return false;
}

TypeResult Parser::ParseCXXSpliceAsType(bool AllowDependent,
                                        bool Complain) {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());
  Expr *Operand = ER.get();

  if (!AllowDependent)
    if (Operand->isTypeDependent() || Operand->isValueDependent())
      return TypeError();

  TypeResult Result = Actions.ActOnCXXSpliceExpectingType(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc(),
          Complain);
  if (!Result.isInvalid())
    ConsumeAnnotationToken();

  return Result;
}

ExprResult Parser::ParseCXXSpliceAsExpr(bool AllowMemberReference) {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());

  ExprResult Result = Actions.ActOnCXXSpliceExpectingExpr(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc(),
          AllowMemberReference);
  if (!Result.isInvalid())
    ConsumeAnnotationToken();

  return Result;
}

DeclResult Parser::ParseCXXSpliceAsNamespace() {
  assert(Tok.is(tok::annot_splice) && "expected annot_splice");

  Token Splice = Tok;
  ConsumeAnnotationToken();

  ExprResult ER = getExprAnnotation(Splice);
  assert(!ER.isInvalid());

  DeclResult Result = Actions.ActOnCXXSpliceExpectingNamespace(
          Splice.getLocation(), ER.get(), Splice.getAnnotationEndLoc());

  return Result;
}
