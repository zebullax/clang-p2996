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
// RUN: %clang_cc1 %s -std=c++23 -freflection

using info = decltype(^int);

namespace myns {
  namespace Inner {}
  struct Cls {};
  template <typename T> struct TCls {
    void memfn();
  };
}  // namespace myns

                                 // ===========
                                 // idempotency
                                 // ===========

namespace idempotency {
static_assert(^[:^myns::TCls:] == ^myns::TCls);
static_assert(^[:^myns:] == ^myns);
static_assert(^[:^:::] == ^::);

static_assert(^[:^myns:]::Cls == ^myns::Cls);
static_assert(^[:^myns:]::TCls<int> == ^myns::TCls<int>);

static_assert(^[:^myns::TCls<int>:]::memfn == ^myns::TCls<int>::memfn);
static_assert(^[:^myns::TCls<int>:]::memfn != ^myns::TCls<bool>::memfn);

static_assert(^[:^myns:]::TCls == ^myns::TCls);
static_assert(^[:^myns:]::Inner == ^myns::Inner);
}  // namespace idempotency
