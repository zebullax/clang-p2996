//===--- Reflection.cpp - Classes for representing reflection ---*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements the ReflectionValue class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/LocInfoType.h"
#include "clang/AST/Reflection.h"

namespace clang {

ReflectionValue::ReflectionValue() : Kind(RK_null), Entity(nullptr) { }

ReflectionValue::ReflectionValue(ReflectionKind Kind, void *Entity)
    : Kind(Kind), Entity(Entity) {
}

ReflectionValue::ReflectionValue(ReflectionValue const& Rhs)
    : Kind(Rhs.Kind), Entity(Rhs.Entity) {
}

ReflectionValue &ReflectionValue::operator=(ReflectionValue const&Rhs) {
  Kind = Rhs.Kind;
  Entity = Rhs.Entity;

  return *this;
}

bool ReflectionValue::isNull() const { return Kind == RK_null; }

QualType ReflectionValue::getAsType() const {
  assert(getKind() == RK_type && "not a type");

  QualType QT = QualType::getFromOpaquePtr(Entity);

  bool UnwrapAliases = false;
  bool IsConst = QT.isConstQualified();
  bool IsVolatile = QT.isVolatileQualified();

  void *AsPtr;
  do {
    AsPtr = QT.getAsOpaquePtr();

    if (const auto *LIT = dyn_cast<LocInfoType>(QT))
      QT = LIT->getType();
    if (const auto *ET = dyn_cast<ElaboratedType>(QT)) {
      QualType New = ET->getNamedType();
      New.setLocalFastQualifiers(QT.getLocalFastQualifiers());
      QT = New;
    }
    if (const auto *STTPT = dyn_cast<SubstTemplateTypeParmType>(QT);
        STTPT && !STTPT->isDependentType())
      QT = STTPT->getReplacementType();
    if (const auto *RST = dyn_cast<ReflectionSpliceType>(QT);
        RST && !RST->isDependentType())
      QT = RST->getUnderlyingType();
    if (const auto *TST = dyn_cast<TemplateSpecializationType>(QT);
        TST && !TST->isTypeAlias()) {
      QT = TST->desugar();
    }
    if (const auto *DTST = dyn_cast<DeducedTemplateSpecializationType>(QT))
      QT = DTST->getDeducedType();
    if (const auto *DTT = dyn_cast<DecltypeType>(QT)) {
      QT = DTT->desugar();
      UnwrapAliases = true;
    }
    if (const auto *UT = dyn_cast<UsingType>(QT);
        UT && UnwrapAliases)
      QT = UT->desugar();
    if (const auto *TDT = dyn_cast<TypedefType>(QT);
        TDT && UnwrapAliases)
      QT = TDT->desugar();
  } while (QT.getAsOpaquePtr() != AsPtr);

  if (IsConst)
    QT = QT.withConst();
  if (IsVolatile)
    QT = QT.withVolatile();

  return QT;
}

void ReflectionValue::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(Kind);
  switch (Kind) {
  case RK_null:
    break;
  case RK_type: {
    // Quals | Kind/0 | ...

    QualType QT = getAsType();

    QT.getQualifiers().Profile(ID);

    if (auto *TST = dyn_cast<TemplateSpecializationType>(QT)) {
      // Note: This sugar only kept for alias template specializations.
      ID.AddInteger(Type::TemplateSpecialization);
      ID.AddPointer(TST->getTemplateName().getAsTemplateDecl());
      ID.AddPointer(QT->getAsRecordDecl()->getCanonicalDecl());
    } else {
      ID.AddInteger(0);
      if (auto *TDT = dyn_cast<TypedefType>(QT)) {
        ID.AddBoolean(true);
        ID.AddPointer(TDT->getDecl());
      } else {
        ID.AddBoolean(false);
        QT.getCanonicalType().Profile(ID);
      }
    }
    break;
  }
  case RK_expr_result: {
    APValue V = getAsExprResult()->getAPValueResult();

    getAsExprResult()->getType()
        .getCanonicalType().getUnqualifiedType().Profile(ID);
    V.Profile(ID);
    break;
  }
  case RK_declaration:
    if (auto *PVD = dyn_cast<ParmVarDecl>(getAsDecl())) {
      auto *FD = cast<FunctionDecl>(PVD->getDeclContext());
      FD = FD->getFirstDecl();
      PVD = FD->getParamDecl(PVD->getFunctionScopeIndex());

      ID.AddPointer(PVD);
    } else {
      ID.AddPointer(getAsDecl());
    }
    break;
  case RK_template:
    ID.AddPointer(getAsTemplate().getAsTemplateDecl()->getCanonicalDecl());
    break;
  case RK_namespace:
    ID.AddPointer(getAsNamespace());
    break;
  case RK_base_specifier:
    ID.AddPointer(getAsBaseSpecifier());
    break;
  case RK_data_member_spec: {
    TagDataMemberSpec *TDMS = getAsDataMemberSpec();
    TDMS->Ty.Profile(ID);
    ID.AddBoolean(TDMS->Name.has_value());
    if (TDMS->Name)
      ID.AddString(TDMS->Name.value());
    ID.AddBoolean(TDMS->Alignment.has_value());
    if (TDMS->Alignment)
      ID.AddBoolean(TDMS->Alignment.value());
    ID.AddBoolean(TDMS->BitWidth.has_value());
    if (TDMS->BitWidth)
      ID.AddBoolean(TDMS->BitWidth.value());
    break;
  }
  }
}

bool ReflectionValue::operator==(ReflectionValue const& Rhs) const {
  llvm::FoldingSetNodeID LID, RID;
  Profile(LID);
  Rhs.Profile(RID);

  return LID == RID;
}

bool ReflectionValue::operator!=(ReflectionValue const& Rhs) const {
  return !(*this == Rhs);
}

bool TagDataMemberSpec::operator==(TagDataMemberSpec const &Rhs) const {
  return (Ty == Ty &&
          Alignment == Rhs.Alignment &&
          BitWidth == Rhs.BitWidth &&
          Name == Rhs.Name);
}

bool TagDataMemberSpec::operator!=(TagDataMemberSpec const &Rhs) const {
  return !(*this == Rhs);
}

}  // end namespace clang
