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
// RUN: %clang_cc1 %s -std=c++26 -fconsteval-blocks


                          // ===========
                          // empty_cases
                          // ===========

namespace empty_cases {

consteval { }

struct S {
  consteval { }
};

class C {
  consteval { }
};

union U {
  consteval { }
};

void fn() {
  consteval { }
}

template <typename>
class TCls {
  consteval { }
};

template <typename>
void TFn() {
  consteval { }
}

}  // namespace empty_cases

                               // ===============
                               // non_empty_cases
                               // ===============

namespace non_empty_cases {
// Consteval blocks aren't very useful without side-effects, which require
// something like std::meta::define_class from the standard library.
//
// Still, we can check to make sure that various things parse.

consteval void fn() {}
struct S1 {
  static consteval void memfn() {}
};
struct S2 {
  static consteval void memfn() {}
};

consteval {
  fn();
  S1::memfn();
}

struct S {
  consteval {
    fn();
    S1::memfn();
  }
};

template <typename... Ts>
struct TCls {
  consteval {
    (Ts::memfn(), ...);
  }
};

TCls<S1, S2> tcls;

}  // non_empty_cases
