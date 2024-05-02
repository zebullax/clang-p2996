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
// RUN: %clang_cc1 %s -std=c++26 -fexpansion-statements


                          // ================
                          // basic_expansions
                          // ================

namespace basic_expansions {

void runtime_fn(auto) {}
consteval void compiletime_fn(auto) {}

int run_tests() {
  // empty expansions are legal, but should not expand.
  template for (auto e : {}) static_assert(false);
  template for (constexpr auto e : {}) static_assert(false);
  template for (int init = 0; auto e : {}) static_assert(false);
  template for (constexpr auto init = 0; auto e : {}) static_assert(false);

  // basic expansions with runtime expansion variables.
  template for (auto e : {1, 'a', false})
    runtime_fn(e);
  template for (int init = 0; int e : {1, 2, 3})
    runtime_fn((init++) + e);

  // basic expansions with constexpr expansion variables.
  template for (constexpr auto e : {1, 'a', false})
    compiletime_fn(e);
  template for (int init = 0; constexpr int e : {1, 2, 3})
    runtime_fn(e + init);

  return 0;
}
}  // namespace basic_expansions

                           // =======================
                           // in_constant_evaluations
                           // =======================

namespace in_constant_evaluations {

consteval int fn() {
  int result = 0;
  template for (result = 5; int e : {1, 3, 5, 7, 9})
    result += e;

  return result;
}
static_assert(fn() == 30);
}  // namespace in_constant_evaluations

                              // =================
                              // initializer_lists
                              // =================

namespace initializer_lists {

struct S { int first, second; };

consteval S fn() {
    S result = {0, 0};
    template for (S s : {{1, 2}, {.first=2, .second=3}}) {
        result.first += s.first;
        result.second += s.second;
    }
    return result;
}
constexpr S s = fn();
static_assert(s.first == 3);
static_assert(s.second == 5);
}  // namespace initializer_lists

                            // ====================
                            // dependent_expansions
                            // ====================

namespace dependent_expansions {

template <typename T>
struct S {
  static constexpr int dependent_type_fn() {
    int result = 0;
    template for (using Alias = T; Alias e : {1, 2, 3})
      result += e;
    return result;
  }

  template <T V>
  static constexpr int dependent_element_in_list_of_known_size_fn() {
    int result = 0;
    template for (auto e : {1, V, 2})
      result += e;
    return result;
  }

  template <auto... Vs>
  static constexpr int dependent_pack_fn() {
    int result = 0;
    template for (auto e : {1, Vs..., 100})
      result += static_cast<int>(e);
    return result;
  }

  static constexpr int dependent_type_parm_var_fn(auto Base) {
    int result = 0;
    template for (auto e : {1, 2, 3})
      result += e * Base;
    return result;
  }
};

static_assert(S<int>::dependent_type_fn() == 6);
static_assert(S<int>::dependent_element_in_list_of_known_size_fn<10>() == 13);
static_assert(S<int>::dependent_pack_fn<1, 2.0f, 3l>() == 107);
static_assert(S<int>::dependent_type_parm_var_fn(10) == 60);

void run_tests() {
  (void) S<int>::dependent_type_fn();
  (void) S<int>::dependent_element_in_list_of_known_size_fn<0>();
  (void) S<int>::dependent_pack_fn<1, 2.0f, 3l>();
  (void) S<int>::dependent_type_parm_var_fn(10);
}
}  // dependent_expansions

                         // ===========================
                         // nested_expansion_statements
                         // ===========================

namespace nested_expansion_statements {

template <auto... Vs>
constexpr int nested_expansion() {
  int result = 0;
  template for (auto r : {1, 2, 3, 4})
    template for (auto s : {Vs...})
      result += r * s;
  return result;
}
static_assert(nested_expansion<4, 3, 2, 1>() == 100);

void run_tests() {
  (void) nested_expansion<4, 3, 2, 1>();
}

}  // nested_expansion_statements
