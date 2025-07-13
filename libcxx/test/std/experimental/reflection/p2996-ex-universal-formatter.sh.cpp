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
// ADDITIONAL_COMPILE_FLAGS: -freflection -fexpansion-statements
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <meta>
#include <format>
#include <print>


struct universal_formatter {
  constexpr auto parse(auto& ctx) { return ctx.begin(); }

  template <typename T>
  auto format(T const& t, auto& ctx) const {
    using std::meta::access_context;

    auto out = std::format_to(ctx.out(), "{}{{", identifier_of(^^T));

    auto delim = [first=true, &out]() mutable {
      if (!first) {
        *out++ = ',';
        *out++ = ' ';
      }
      first = false;
    };

    template for (constexpr auto base :
                  define_static_array(bases_of(^^T,
                                               access_context::current()))) {
        delim();
        out = std::format_to(out, "{}",
                             (typename [: type_of(base) :] const&)(t));
    };

    template for (constexpr auto mem :
                  define_static_array(
                      nonstatic_data_members_of(^^T,
                                                access_context::current()))) {
      delim();
      out = std::format_to(out, ".{}={}", identifier_of(mem), t.[:mem:]);
    };

    *out++ = '}';
    return out;
  }
};

struct B { int m0 = 0; };
struct X : B { int m1 = 1; };
struct Y : B { int m2 = 2; };
class Z : public X, private Y {
  [[maybe_unused]] int m3 = 3;
  [[maybe_unused]] int m4 = 4;

  friend struct universal_formatter;
};

template <> struct std::formatter<B> : universal_formatter { };
template <> struct std::formatter<X> : universal_formatter { };
template <> struct std::formatter<Y> : universal_formatter { };
template <> struct std::formatter<Z> : universal_formatter { };

int main() {
  // RUN: grep "Z{X{B{.m0=0}, .m1=1}, Y{B{.m0=0}, .m2=2}, .m3=3, .m4=4}" \
  // RUN:     %t.stdout | wc -l
  std::println("{}", Z());
}
