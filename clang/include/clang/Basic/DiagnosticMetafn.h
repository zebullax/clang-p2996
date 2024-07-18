//==--- DiagnosticMetafn - Diagnostics for P2996 metafunctions ------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_DIAGNOSTICMETAFN_H
#define LLVM_CLANG_BASIC_DIAGNOSTICMETAFN_H

#include "clang/Basic/Diagnostic.h"

namespace clang {
namespace diag {
enum {
#define DIAG(ENUM, FLAGS, DEFAULT_MAPPING, DESC, GROUP, SFINAE, NOWERROR,      \
             SHOWINSYSHEADER, SHOWINSYSMACRO, DEFERRABLE, CATEGORY)            \
  ENUM,
#define METAFNSTART
#include "clang/Basic/DiagnosticMetafnKinds.inc"
#undef DIAG
  NUM_BUILTIN_METAFN_DIAGNOSTICS
};
} // end namespace diag
} // end namespace clang

#endif // LLVM_CLANG_BASIC_DIAGNOSTICMETAFN_H
