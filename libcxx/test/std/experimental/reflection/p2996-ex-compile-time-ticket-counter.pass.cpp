//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <meta>

#include <print>


template<int N> struct Helper;

struct TU_Ticket {
  static consteval int latest() {
    int k = 0;
    while (is_complete_type(substitute(^^Helper,
                                       { std::meta::reflect_constant(k) })))
      ++k;
    return k;
  }

  static consteval void increment() {
    define_aggregate(substitute(^^Helper,
                                { std::meta::reflect_constant(latest())}),
                     {});
  }
};

constexpr int x = TU_Ticket::latest();  // x initialized to 0.

consteval { TU_Ticket::increment(); }
constexpr int y = TU_Ticket::latest();  // y initialized to 1.

consteval { TU_Ticket::increment(); }
constexpr int z = TU_Ticket::latest();  // z initialized to 2.

int main() {
  // RUN: grep "0, 1, 2" %t.stdout
  std::println("{}, {}, {}", x, y, z);
}
