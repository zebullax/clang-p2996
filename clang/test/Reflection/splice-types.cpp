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


template <typename T1, typename T2>
constexpr bool is_same_v = false;

template <typename T1>
constexpr bool is_same_v<T1, T1> = true;

                                 // ===========
                                 // idempotency
                                 // ===========

// Check idempotency of the splice operator composed with reflection.
namespace idempotency {
static_assert(is_same_v<typename [:^int:], int>);
static_assert(is_same_v<typename [:^const int:], const int>);
static_assert(!is_same_v<typename [:^const int:], int>);
}  // namespace idempotency

                                   // =======
                                   // aliases
                                   // =======

// Check use of splices in alias definitions.
namespace aliases {
constexpr info r_int_alias = ^int, r_const_int_alias = ^const int;
using int_alias = [:r_int_alias:];
using const_int_alias = [:r_const_int_alias:];
static_assert(is_same_v<typename [:r_int_alias:], int>);
static_assert(is_same_v<typename [:r_int_alias:], int_alias>);
static_assert(is_same_v<typename [:r_const_int_alias:], const int>);
static_assert(is_same_v<typename [:r_const_int_alias:], const_int_alias>);
static_assert(!is_same_v<typename [:r_int_alias:],
                         typename [:r_const_int_alias:]>);
}  // namespace aliases

                                  // ========
                                  // in_decls
                                  // ========

// Check use of splices in declarations.
namespace in_decls {
  constexpr info r_const_int = ^const int;
  constexpr [:r_const_int:] x = 42;
  static_assert(is_same_v<decltype(x), const int>);
  static_assert(x == 42);
}  // namespace in_decls

                                 // ==========
                                 // in_fn_defs
                                 // ==========

// Check use of splices in function return and parameter types.
namespace in_fn_defs {
constexpr info r_int = ^int;
constexpr info r_const_int = ^const int;
consteval typename [:r_const_int:] incr(typename [:r_int:] p) {
  return p + 1;
}
static_assert(incr(14) == 15);

consteval auto decr(typename [:r_int:] p) -> [:r_const_int:] {
  return p - 1;
}
static_assert(decr(13) == 12);
}  // in_fn_defs

                                   // ======
                                   // in_nns
                                   // ======

// Check use of splices as leading components of nested name specifiers.
namespace in_nns {
struct S {
  struct Inner {
    int k;

    using type = int;
    static constexpr int s_value = 1;
  } I;
  using type = bool;
  static constexpr int s_value = 2;
};
constexpr auto r_S = ^S;
constexpr auto r_S_Inner = ^S::Inner;
static_assert(is_same_v<[:r_S:]::type, bool>);
static_assert(is_same_v<[:r_S_Inner:]::type, int>);
static_assert([:r_S:]::s_value == 2);
static_assert([:r_S_Inner:]::s_value == 1);

// Check splice as leading component of nested name specifier appearing in the
// RHS of a member access expression.
consteval int fn(int p, int q) {
  S s;
  s.I.[:r_S:]::Inner::k = p;
  s.I.[:r_S_Inner:]::k += q;
  return s.I.k;
}
static_assert(fn(3, 4) == 7);

// Check splice as leading component of nested name specifier appearing in the
// lookup of an inner class type or static data member.
template <info T, typename ExpectedMemberTypedef, int ExpectedMemberValue>
consteval bool fn_with_dependent() {
  return (is_same_v<typename [:T:]::type, ExpectedMemberTypedef>) &&
         ([:T:]::s_value == ExpectedMemberValue);
}
static_assert(fn_with_dependent<^S, bool, 2>());
static_assert(fn_with_dependent<^S::Inner, int, 1>());

// Check splice as leading component of nested name specifier appearing in the
// RHS of a member access expression when the reflection is value-dependent on
// a template parameter name.
template <info T>
consteval int fn_with_dependent() {
  S s;
  s.I.[:T:]::k = 13;

  return s.I.k;
}
static_assert(fn_with_dependent<^S::Inner>() == 13);

// Check composition of splice and reflect operators to obtain a reflection to
// an inner class belonging to a reflected class, in which the reflection is
// value-dependent on a template parameter name.
template <info T>
consteval info getReflectionOfInnerCls() {
  return ^typename [:T:]::Inner;
}
static_assert(getReflectionOfInnerCls<^S>() == ^S::Inner);
}  // namespace in_nns

                               // ===============
                               // with_enum_types
                               // ===============

// Check splicing of enum types.
namespace with_enum_types {
enum Enum { A, B, C };
enum class EnumCls { A, B, C };

constexpr info rEnum = ^Enum, rEnumCls = ^EnumCls;
static_assert([:rEnum:]::B == B);
static_assert([:rEnumCls:]::B == EnumCls::B);
static_assert(static_cast<Enum>([:rEnumCls:]::B) == [:rEnum:]::B);

// Splicing of a dependent reflection of an enum class.
template <info R> consteval int bval() { return int([:R:]::B); }
static_assert(bval<^Enum>() == bval<^EnumCls>());

// using-enum-declaration with a splice.
namespace {
using enum [:rEnumCls:];
static_assert(int(C) == 2);
}  // namespace

// using-enum-declaration with a qualified-id having a splice as the leading
// nested-name-specifier.
namespace {
constexpr auto rns = ^with_enum_types;
using enum [:rns:]::EnumCls;
static_assert(int(B) == 1);
}  // namespace
}  // namespace with_enum_types

                             // ===================
                             // friend_declarations
                             // ===================

namespace friend_declarations {
struct S;

template <decltype(^::) FriendCls>
class T {
  static constexpr int value = 13;

  friend [:FriendCls:];
};

struct S {
  static constexpr auto value = T<^S>::value;
};

static_assert(S::value == 13);
}  // namespace friend_declarations
