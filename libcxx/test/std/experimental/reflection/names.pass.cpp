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

#include <experimental/meta>


static_assert(name_of(^::) == "");
static_assert(name_of(^3) == "");
static_assert(name_of(^int) == "int");
static_assert(display_name_of(^::) == "");
static_assert(display_name_of(^3) == "");
static_assert(display_name_of(^int) == "int");


using Alias = int;
int var;
void fn();
[[maybe_unused]] void sfn();
template <typename> struct TCls {};
template <typename P, auto P2, template<typename> class P3> void TFn();
template <typename> using TAlias = int;
template <typename> int TVar = 0;
template <typename> concept Concept = requires { true; };
template <typename...> struct WithTypePack {};
template <auto...> struct WithAutoPack {};
enum Enum { A };
enum class EnumCls { A };

static_assert(name_of(^Alias) == "Alias");
static_assert(name_of(^var) == "var");
static_assert(name_of(^fn) == "fn");
static_assert(name_of(^TCls) == "TCls");
static_assert(name_of(^TFn) == "TFn");
static_assert(name_of(^TAlias) == "TAlias");
static_assert(name_of(^TVar) == "TVar");
static_assert(name_of(^Concept) == "Concept");
static_assert(name_of(^Enum) == "Enum");
static_assert(name_of(^Enum::A) == "A");
static_assert(name_of(^EnumCls) == "EnumCls");
static_assert(name_of(^EnumCls::A) == "A");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) == "int");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) == "");
static_assert(name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) == "TCls");
static_assert(name_of(template_arguments_of(^WithTypePack<int>)[0]) == "int");
static_assert(name_of(template_arguments_of(^WithAutoPack<3>)[0]) == "");
static_assert(display_name_of(^::) == "");
static_assert(display_name_of(^var) == "var");
static_assert(display_name_of(^fn) == "fn");
static_assert(display_name_of(^TCls) == "TCls");
static_assert(display_name_of(^TFn) == "TFn");
static_assert(display_name_of(^TAlias) == "TAlias");
static_assert(display_name_of(^TVar) == "TVar");
static_assert(display_name_of(^Concept) == "Concept");
static_assert(display_name_of(^Enum) == "Enum");
static_assert(display_name_of(^Enum::A) == "A");
static_assert(display_name_of(^EnumCls) == "EnumCls");
static_assert(display_name_of(^EnumCls::A) == "A");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[0]) ==
              "int");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[1]) ==
              "");
static_assert(display_name_of(template_arguments_of(^TFn<int, 0, TCls>)[2]) ==
              "TCls");
static_assert(display_name_of(template_arguments_of(^WithTypePack<int>)[0]) ==
              "int");
static_assert(display_name_of(template_arguments_of(^WithAutoPack<3>)[0]) ==
              "");


struct Base {};
struct Cls : Base {
  using Alias = int;

  int mem;
  void memfn() const;
  static void sfn();
  struct Inner { int inner_mem; };

  Cls();
  template <typename T> Cls();
  ~Cls();

  operator bool();

  template <typename> struct TInner {};
  template <typename> void TMemFn();
  template <typename> using TAlias = int;
  template <typename> static constexpr int TSVar = 0;
  template <typename> operator int();

  enum Enum { B };
  enum class EnumCls { B };
};
static_assert(name_of(^Cls) == "Cls");
static_assert(name_of(bases_of(^Cls)[0]) == "Base");
//static_assert(name_of(^Cls::Alias) == "Alias");  // TODO(P2996).
static_assert(name_of(^Cls::mem) == "mem");
static_assert(name_of(^Cls::memfn) == "memfn");
static_assert(name_of(^Cls::sfn) == "sfn");
//static_assert(name_of(^Cls::Inner) == "Inner");  // TODO(P2996).
static_assert(name_of(members_of(^Cls, std::meta::is_constructor)[0]) == "Cls");
static_assert(name_of(members_of(^Cls, std::meta::is_constructor)[1]) == "Cls");
static_assert(name_of(members_of(^Cls, std::meta::is_destructor)[0]) == "~Cls");
static_assert(name_of(^Cls::operator bool) == "operator bool");
static_assert(name_of(members_of(^Cls, std::meta::is_template)[5]) ==
              "operator int");
//static_assert(name_of(^Cls::TInner) == "TInner");  // TODO(P2996).
//static_assert(name_of(^Cls::TMemFn) == "TMemFn");  // TODO(P2996).
//static_assert(name_of(^Cls::TAlias) == "TAlias");  // TODO(P2996).
//static_assert(name_of(^Cls::TSVar) == "TSVar");  // TODO(P2996).
//static_assert(name_of(^Cls::Enum) == "Enum");  // TODO(P2996).
static_assert(name_of(^Cls::Enum::B) == "B");
//static_assert(name_of(^Cls::EnumCls) == "EnumCls");  // TODO(P2996).
static_assert(name_of(^Cls::EnumCls::B) == "B");
static_assert(display_name_of(^Cls) == "Cls");
static_assert(display_name_of(bases_of(^Cls)[0]) == "Base");
//static_assert(display_name_of(^Cls::Alias) == "Alias");  // TODO(P2996).
static_assert(display_name_of(^Cls::mem) == "mem");
static_assert(display_name_of(^Cls::memfn) == "memfn");
static_assert(display_name_of(^Cls::sfn) == "sfn");
//static_assert(display_name_of(^Cls::Inner) == "Inner");  // TODO(P2996).
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_constructor)[0]) == "Cls");
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_constructor)[1]) == "Cls");
static_assert(display_name_of(members_of(^Cls,
                              std::meta::is_destructor)[0]) == "~Cls");
static_assert(display_name_of(^Cls::operator bool) == "operator bool");
static_assert(display_name_of(members_of(^Cls, std::meta::is_template)[5]) ==
              "operator int");
//static_assert(display_name_of(^Cls::TInner) == "TInner");  // TODO(P2996).
//static_assert(display_name_of(^Cls::TMemFn) == "TMemFn");  // TODO(P2996).
//static_assert(display_name_of(^Cls::TAlias) == "TAlias");  // TODO(P2996).
//static_assert(display_name_of(^Cls::TSVar) == "TSVar");  // TODO(P2996).
//static_assert(display_name_of(^Cls::Enum) == "Enum");  // TODO(P2996).
static_assert(display_name_of(^Cls::Enum::B) == "B");
//static_assert(display_name_of(^Cls::EnumCls) == "EnumCls");  // TODO(P2996).
static_assert(display_name_of(^Cls::EnumCls::B) == "B");



namespace myns {
int mem;
void memfn();
struct Cls { int mem; };

template <typename> struct TInner {};
template <typename> void TFn();
template <typename> using TAlias = int;
template <typename> int TVar = 0;
template <typename> concept Concept = requires { true; };

enum Enum { C };
enum class EnumCls { C };
}  // namespace myns
static_assert(name_of(^myns) == "myns");
static_assert(name_of(^myns::mem) == "mem");
static_assert(name_of(^myns::memfn) == "memfn");
//static_assert(name_of(^myns::Cls) == "Cls");  // TODO(P2996).
//static_assert(name_of(^myns::TInner) == "TInner");  // TODO(P2996).
//static_assert(name_of(^myns::TFn) == "TFn");  // TODO(P2996).
//static_assert(name_of(^myns::TAlias) == "TAlias");  // TODO(P2996).
//static_assert(name_of(^myns::TVar) == "TVar");  // TODO(P2996).
//static_assert(name_of(^myns::Concept) == "Concept");  // TODO(P2996).
//static_assert(name_of(^myns::Enum) == "Enum");  // TODO(P2996).
static_assert(name_of(^myns::Enum::C) == "C");
//static_assert(name_of(^myns::EnumCls) == "EnumCls");  // TODO(P2996).
static_assert(name_of(^myns::EnumCls::C) == "C");
static_assert(display_name_of(^myns) == "myns");
static_assert(display_name_of(^myns::mem) == "mem");
static_assert(display_name_of(^myns::memfn) == "memfn");
//static_assert(display_name_of(^myns::Cls) == "Cls");  // TODO(P2996).
//static_assert(display_name_of(^myns::TInner) == "TInner");  // TODO(P2996).
//static_assert(display_name_of(^myns::TFn) == "TFn");  // TODO(P2996).
//static_assert(display_name_of(^myns::TAlias) == "TAlias");  // TODO(P2996).
//static_assert(display_name_of(^myns::TVar) == "TVar");  // TODO(P2996).
//static_assert(display_name_of(^myns::Concept) == "Concept");  // TODO(P2996).
//static_assert(display_name_of(^myns::Enum) == "Enum");  // TODO(P2996).
static_assert(display_name_of(^myns::Enum::C) == "C");
//static_assert(display_name_of(^myns::EnumCls) == "EnumCls");  // TODO(P2996).
static_assert(display_name_of(^myns::EnumCls::C) == "C");


int main() { }
