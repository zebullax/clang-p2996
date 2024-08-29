//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection -freflection-new-syntax \
// RUN: -fannotation-attributes

using info = decltype(^^int);

                                // =============
                                // basic_parsing
                                // =============

namespace basic_parsing {

consteval int fn() { return 1; }

[[maybe_unused, =42, =basic_parsing::fn(), maybe_unused]]
void annFn();

struct [[maybe_unused, =42, =basic_parsing::fn(), maybe_unused]] S;

template <typename>
  struct [[maybe_unused, =42, =basic_parsing::fn(), maybe_unused]] TCls;

namespace [[maybe_unused, =42, =basic_parsing::fn(), maybe_unused]] NS {};

}  // namespace basic_parsing
