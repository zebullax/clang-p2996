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
template <int K> struct TCls {
  static constexpr int value = K;
};

                       // ==============================
                       // reflections_of_argument_values
                       // ==============================

namespace reflections_of_argument_values {
static_assert([:std::meta::reflect_result(42):] == 42);
static_assert([:std::meta::reflect_result(EnumCls::A):] == EnumCls::A);
static_assert([:std::meta::reflect_result(ConstVar):] == ConstVar);
static_assert([:std::meta::reflect_result(ConstexprVar):] == ConstexprVar);
static_assert([:std::meta::reflect_result(fn):] == &fn);
static_assert([:std::meta::reflect_result(&Cls::k):] == &Cls::k);
static_assert([:std::meta::reflect_result(&Cls::fn):] == &Cls::fn);
}  // namespace reflections_of_argument_values

                       // ==============================
                       // values_of_reflection_arguments
                       // ==============================

namespace values_of_reflection_arguments {
template <auto Expected>
consteval bool CheckValueIs(std::meta::info R) {
  return extract<decltype(Expected)>(R) == Expected;
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

[[maybe_unused]] TCls<3> ignored;
static_assert(CheckValueIs<3>(static_data_members_of(substitute(^TCls,
                                                                {^3}))[0]));
}  // namespace values_of_reflection_arguments

                                  // =========
                                  // roundtrip
                                  // =========

namespace roundtrip {
template <typename T>
consteval bool Roundtrip(T value) {
  return extract<T>(std::meta::reflect_result(value)) == value;
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

                            // =====================
                            // extract_ref_semantics
                            // =====================

namespace extract_ref_semantics {
  int nonConstGlobal = 1;
  const int constGlobal = 2;

  static_assert(&extract<int &>(^nonConstGlobal) == &nonConstGlobal);
  static_assert(extract<int &>(^constGlobal) == 2);

  const int &constGlobalRef = constGlobal;
  static_assert(&extract<const int &>(^constGlobalRef) == &constGlobal);

  // TODO(P2996): Need to decide whether 'extract' should be legal on locals
  // within an immediate function context. It isn't legal as spec'd by P2996R3,
  // but we may want to make it work. Not going to sink more time into making
  // the commented line below work, until we have a decision.
  consteval int myfn(int arg) {
    int val = 3;
    int &ref = extract<int &>(^val);

    //int &ref2 = extract<int &>(^ref);
    //static_assert(&ref2 == &val);

    ref = 4;
    return val + extract<int>(^arg);
  }
  static_assert(myfn(5) == 9);

  consteval const int &returnsRef() { return constGlobal; }

  void nonConstFn() {
    constexpr auto r = ^returnsRef();
    static_assert(extract<const int &>(r) == 2);
  }
}  // namespace extract_ref_semantics

                            // ====================
                            // reflect_result_types
                            // ====================

namespace reflect_result_types {
struct S {};

int v = 3;
const int cv = 3;
S s;
const S cs;

static_assert(type_of(std::meta::reflect_result(3)) == ^int);
static_assert(type_of(std::meta::reflect_result<const int>(3)) == ^int);

static_assert(type_of(std::meta::reflect_result<int&>(v)) == ^int&);
static_assert(type_of(std::meta::reflect_result<const int&>(v)) == ^const int&);

static_assert(type_of(std::meta::reflect_result(cv)) == ^int);
static_assert(type_of(std::meta::reflect_result<int>(cv)) == ^int);
static_assert(type_of(std::meta::reflect_result<const int>(cv)) == ^int);
static_assert(type_of(std::meta::reflect_result<const int&>(cv)) == ^const int&);

static_assert(type_of(std::meta::reflect_result<S&>(s)) == ^S&);
static_assert(type_of(std::meta::reflect_result<const S&>(s)) == ^const S&);

static_assert(type_of(std::meta::reflect_result(cs)) == ^S);
static_assert(type_of(std::meta::reflect_result<S>(cs)) == ^S);
static_assert(type_of(std::meta::reflect_result<const S>(cs)) == ^const S);
static_assert(type_of(std::meta::reflect_result<const S&>(cs)) == ^const S&);
}  // namespace reflect_result_types

                        // ============================
                        // reflect_result_ref_semantics
                        // ============================

namespace reflect_result_ref_semantics {
  int nonConstGlobal = 1;
  const int constGlobal = 2;

  constexpr auto r = std::meta::reflect_result<const int &>(constGlobal);
  static_assert(type_of(r) == ^const int &);
  static_assert([:r:] == 2);
}  // namespace reflect_result_ref_semantics

                             // ===================
                             // values_from_objects
                             // ===================

namespace values_from_objects {

const int constGlobal = 11;
constexpr auto rref = std::meta::reflect_result<const int &>(constGlobal);

static_assert(value_of(^constGlobal) != ^constGlobal);
static_assert([:value_of(^constGlobal):] == 11);
static_assert([:value_of(rref):] == 11);
static_assert(value_of(^constGlobal) == value_of(rref));
static_assert(value_of(^constGlobal) == std::meta::reflect_result(11));

enum Enum { A };
static constexpr Enum e = A;

enum EnumCls { CA };
static constexpr EnumCls ce = CA;

static_assert(value_of(^A) != value_of(^CA));

static_assert(value_of(^A) != std::meta::reflect_result(0));
static_assert(value_of(^A) == std::meta::reflect_result(Enum(0)));
static_assert(value_of(^A) != std::meta::reflect_result(EnumCls(0)));
static_assert(value_of(^e) != std::meta::reflect_result(0));
static_assert(value_of(^e) == std::meta::reflect_result(Enum(0)));
static_assert(value_of(^e) != std::meta::reflect_result(EnumCls(0)));
static_assert(value_of(^ce) != std::meta::reflect_result(0));
static_assert(value_of(^ce) != std::meta::reflect_result(Enum(0)));
static_assert(value_of(^ce) == std::meta::reflect_result(EnumCls(0)));

}  // namespace values_from_objects

int main() {
  // RUN: grep "call-lambda-value: 1" %t.stdout
  extract<void(*)(int)>(^[](int id) {
    std::println("call-lambda-value: {}", id);
  })(1);

  // RUN: grep "call-generic-lambda-value: 2 (int)" %t.stdout
  extract<void(*)(int)>(^[](auto id) {
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
  extract<void(*)(int)>(^l)(1);

  // RUN: grep "call-lambda-var: 2" %t.stdout
  extract<decltype(l)>(^l)(2);

  // RUN: grep "call-generic-lambda-var: 3 (int)" %t.stdout
  extract<decltype(g)>(^g)(3);

  // RUN: grep "call-generic-lambda-var: true (bool)" %t.stdout
  extract<void(*)(bool)>(^g)(true);

  // RUN: grep "updated-extract-global: 42" %t.stdout
  int &ref = extract<int &>(^extract_ref_semantics::nonConstGlobal);
  ref = 42;
  std::println("updated-extract-global: {}",
               extract_ref_semantics::nonConstGlobal);

  // RUN: grep "updated-reflect-result-global: 13" %t.stdout
  constexpr auto r = std::meta::reflect_result<int &>(
        reflect_result_ref_semantics::nonConstGlobal);
  static_assert(type_of(r) == ^int &);
  [:r:] = 13;
  std::println("updated-reflect-result-global: {}",
               reflect_result_ref_semantics::nonConstGlobal);
}
