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

#include <experimental/meta>

#include <print>


class TU_Ticket {
  template<int N> struct Helper {
    static constexpr int MyValue = N;
  };
public:
  static consteval int next() {
    // Search for the next incomplete Helper<k>.
    std::meta::info r;
    for (int k = 0;; ++k) {
      r = substitute(^Helper, { std::meta::reflect_value(k) });
      if (is_incomplete_type(r)) break;
    }
    // Return the value of its member.  Calling static_data_members_of
    // triggers the instantiation (i.e., completion) of Helper<k>.
    return value_of<int>(static_data_members_of(r)[0]);
  }
};

int main() {
  // RUN: grep "0, 1, 2" %t.stdout
  std::println("{}, {}, {}",
               TU_Ticket::next(), TU_Ticket::next(), TU_Ticket::next());
}
