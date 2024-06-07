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
// ADDITIONAL_COMPILE_FLAGS: -Wno-inconsistent-missing-override

// <experimental/reflection>
//
// [reflection]
//
// RUN: %{build}
// RUN: %{exec} %t.exe > %t.stdout

#include <experimental/meta>

#include <algorithm>
#include <array>
#include <print>
#include <ranges>
#include <tuple>


                        // ============================
                        // alexandrescu_lambda_to_tuple
                        // ============================

namespace alexandrescu_lambda_to_tuple {
consteval auto struct_to_tuple_type(std::meta::info type) -> std::meta::info {
  constexpr auto remove_cvref = [](std::meta::info r) consteval {
    return substitute(^std::remove_cvref_t, {r});
  };

  return substitute(^std::tuple,
                    nonstatic_data_members_of(type)
                    | std::views::transform(std::meta::type_of)
                    | std::views::transform(remove_cvref)
                    | std::ranges::to<std::vector>());
}

template <typename To, typename From, std::meta::info... members>
constexpr auto struct_to_tuple_helper(From const& from) -> To {
  return To(from.[:members:]...);
}

template<typename From>
consteval auto get_struct_to_tuple_helper() {
  using To = [: struct_to_tuple_type(^From) :];

  std::vector args = {^To, ^From};
  for (auto mem : nonstatic_data_members_of(^From)) {
    args.push_back(reflect_value(mem));
  }

  return extract<To(*)(From const&)>(substitute(^struct_to_tuple_helper, args));
}

template <typename From>
constexpr auto struct_to_tuple(From const& from) {
  return get_struct_to_tuple_helper<From>()(from);
}

void run_test() {
  struct S { bool i; int j; int k; };

  int a = 1;
  bool b = true;
  [[maybe_unused]] int c = 32;
  std::string d = "hello";
  auto fn = [=]() {
    (void) a; (void) b; (void) d;
  };

  // RUN: grep "Lambda state: <1, true, \"hello\">" %t.stdout
  auto t = struct_to_tuple(fn);
  std::println(R"(Lambda state: <{}, {}, "{}">)",
               get<0>(t), get<1>(t), get<2>(t));
}
}  // namespace alexandrescu_lambda_to_tuple

namespace pdimov_sorted_type_list {
template<class...> struct type_list { };

consteval auto sorted_impl( std::vector<std::meta::info> v )
{
    std::ranges::sort( v, {}, std::meta::alignment_of );
    return v;
}

template<class... Ts> consteval auto sorted()
{
    using R = typename [: substitute( ^type_list, sorted_impl({ ^Ts... }) ) :];
    return R{};
}

void run_test() {
  static_assert(typeid(sorted<int, double, char>()) ==
                typeid(type_list<char, int, double>));
}
}  // namespace pdimov_sorted_type_list

                       // ==============================
                       // std_apply_with_function_splice
                       // ==============================

namespace std_apply_with_function_splice {
void run_test() {
  struct S {
    static void print(std::string_view sv) {
      std::println("std::apply: {}", sv);
    }
  };

  // RUN: grep "std::apply: Hello, world!" %t.stdout
  std::apply([:^S::print:], std::tuple {"Hello, world!"});
}
}  // namespace std_apply_with_function_splice

template <std::meta::info... Tests>
void run_tests() {
  (..., [:Tests:]::run_test());
}

int main() {
  run_tests<^alexandrescu_lambda_to_tuple,
            ^std_apply_with_function_splice,
            ^pdimov_sorted_type_list>();
}
