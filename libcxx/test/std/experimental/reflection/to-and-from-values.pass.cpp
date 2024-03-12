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
// ADDITIONAL_COMPILE_FLAGS: -Wno-unneeded-internal-declaration

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <print>


enum Enum { A = 42 };
enum class EnumCls { A = 42 };
const int ConstVar = 42;
constexpr int ConstexprVar = 42;
void fn() {}
struct Cls {
  int k;
  void fn();
};

                       // ==============================
                       // reflections_of_argument_values
                       // ==============================

namespace reflections_of_argument_values {
static_assert([:std::meta::reflect_value(42):] == 42);
static_assert([:std::meta::reflect_value(EnumCls::A):] == EnumCls::A);
static_assert([:std::meta::reflect_value(ConstVar):] == ConstVar);
static_assert([:std::meta::reflect_value(ConstexprVar):] == ConstexprVar);
static_assert([:std::meta::reflect_value(fn):] == &fn);
static_assert([:std::meta::reflect_value(&Cls::k):] == &Cls::k);
static_assert([:std::meta::reflect_value(&Cls::fn):] == &Cls::fn);
}  // namespace reflections_of_argument_values

                       // ==============================
                       // values_of_reflection_arguments
                       // ==============================

namespace values_of_reflection_arguments {
template <auto Expected>
consteval bool CheckValueIs(std::meta::info R) {
  return value_of<decltype(Expected)>(R) == Expected;
}
static_assert(CheckValueIs<42>(^42));
static_assert(CheckValueIs<EnumCls::A>(^EnumCls::A));
static_assert(CheckValueIs<Enum::A>(^Enum::A));
static_assert(CheckValueIs<42>(^ConstVar));
static_assert(CheckValueIs<42>(^ConstexprVar));
static_assert(CheckValueIs<&fn>(^fn));
static_assert(CheckValueIs<&Cls::k>(^Cls::k));
static_assert(CheckValueIs<&Cls::fn>(^Cls::fn));
static_assert(CheckValueIs<42>([]() {
  constexpr int x = 42;
  return ^x;
}()));
}  // namespace values_of_reflection_arguments

                                  // =========
                                  // roundtrip
                                  // =========

namespace roundtrip {
template <typename T>
consteval bool Roundtrip(T value) {
  return value_of<T>(std::meta::reflect_value(value)) == value;
}
static_assert(Roundtrip(42));
static_assert(Roundtrip(EnumCls::A));
static_assert(Roundtrip(Enum::A));
static_assert(Roundtrip(ConstVar));
static_assert(Roundtrip(ConstexprVar));
static_assert(Roundtrip(fn));
static_assert(Roundtrip(&Cls::k));
static_assert(Roundtrip(&Cls::fn));
static_assert(Roundtrip([] { }));
}  // namespace roundtrip

                           // ======================
                           // value_of_ref_semantics
                           // ======================

namespace value_of_ref_semantics {
  int nonConstGlobal = 1;
  const int constGlobal = 2;

  static_assert(&value_of<int &>(^nonConstGlobal) == &nonConstGlobal);
  static_assert(value_of<int &>(^constGlobal) == 2);

  consteval int myfn(int arg) {
    int val = 3;
    int &ref = value_of<int &>(^val);

    ref = 4;
    return val + value_of<int>(^arg);
  }
  static_assert(myfn(5) == 9);
}

int main() {
  // RUN: grep "call-lambda-value: 1" %t.stdout
  value_of<void(*)(int)>(^[](int id) {
    std::println("call-lambda-value: {}", id);
  })(1);

  // RUN: grep "call-generic-lambda-value: 2 (int)" %t.stdout
  value_of<void(*)(int)>(^[](auto id) {
    std::println("call-generic-lambda-value: {} ({})", id,
                 name_of(type_of(^id)));
  })(2);

  constexpr auto l = [](int id) {
    std::println("call-lambda-var: {}", id);
  };

  constexpr auto g = [](auto id) {
    std::println("call-generic-lambda-var: {} ({})", id,
                 name_of(type_of(^id)));
  };

  // RUN: grep "call-lambda-var: 1" %t.stdout
  value_of<void(*)(int)>(^l)(1);

  // RUN: grep "call-lambda-var: 2" %t.stdout
  value_of<decltype(l)>(^l)(2);

  // RUN: grep "call-generic-lambda-var: 3 (int)" %t.stdout
  value_of<decltype(g)>(^g)(3);

  // RUN: grep "call-generic-lambda-var: true (bool)" %t.stdout
  value_of<void(*)(bool)>(^g)(true);

  // RUN: grep "updated-global: 42" %t.stdout
  int &ref = value_of<int &>(^value_of_ref_semantics::nonConstGlobal);
  ref = 42;
  std::println("updated-global: {}", value_of_ref_semantics::nonConstGlobal);
}
