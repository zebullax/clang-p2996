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
// ADDITIONAL_COMPILE_FLAGS: -freflection-new-syntax
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <experimental/reflection>
//
// [reflection]

#include <experimental/meta>


                                 // ==========
                                 // empty_case
                                 // ==========

namespace empty_case {
void fn();
struct S;
template <typename> void TFn();
template <typename> struct TCls {};
namespace NS {};

static_assert(annotations_of(^^int).size() == 0);
static_assert(annotations_of(^^fn).size() == 0);
static_assert(annotations_of(^^S).size() == 0);
static_assert(annotations_of(^^TFn<int>).size() == 0);
static_assert(annotations_of(^^TCls<int>).size() == 0);
static_assert(annotations_of(^^NS).size() == 0);
}  // namespace empty_case

                                // =============
                                // non_dependent
                                // =============

namespace non_dependent {
[[maybe_unused, =1, =1, =2, =1.0f]] void fn();
struct [[maybe_unused, =3, =3, =4, =2.0f]] S;
template <typename> struct [[maybe_unused, =5, =5, =6, =3.0f]] TCls {};
template <typename> [[maybe_unused, =5, =5, =6, =3.0f]] void TFn();
namespace NS [[maybe_unused, =7, =7, =8, =4.0f]] {}

static_assert((annotations_of(^^fn) |
                  std::views::transform(std::meta::value_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_value(1),
                           std::meta::reflect_value(1),
                           std::meta::reflect_value(2),
                           std::meta::reflect_value(1.0f)});
static_assert((annotations_of(^^S) |
                  std::views::transform(std::meta::value_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_value(3),
                           std::meta::reflect_value(3),
                           std::meta::reflect_value(4),
                           std::meta::reflect_value(2.0f)});
static_assert((annotations_of(^^TCls<int>) |
                  std::views::transform(std::meta::value_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_value(5),
                           std::meta::reflect_value(5),
                           std::meta::reflect_value(6),
                           std::meta::reflect_value(3.0f)});
static_assert((annotations_of(^^TFn<int>) |
                  std::views::transform(std::meta::value_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_value(5),
                           std::meta::reflect_value(5),
                           std::meta::reflect_value(6),
                           std::meta::reflect_value(3.0f)});
static_assert((annotations_of(^^NS) |
                  std::views::transform(std::meta::value_of) |
                  std::ranges::to<std::vector>()) ==
              std::vector {std::meta::reflect_value(7),
                           std::meta::reflect_value(7),
                           std::meta::reflect_value(8),
                           std::meta::reflect_value(4.0f)});

static_assert(is_annotation(annotations_of(^^fn)[0]));
static_assert(is_annotation(annotations_of(^^S)[0]));
static_assert(is_annotation(annotations_of(^^TCls<int>)[0]));
static_assert(is_annotation(annotations_of(^^TFn<int>)[0]));

static_assert(type_of(annotations_of(^^fn)[0]) == ^^int);
static_assert(type_of(annotations_of(^^fn)[3]) == ^^float);

static_assert(annotations_of(^^fn, ^^int).size() == 3);
static_assert(annotations_of(^^fn, ^^float).size() == 1);
static_assert(annotation_of_type<float>(^^fn) == 1.0f);
static_assert(annotation_of_type<char *>(^^fn) == std::nullopt);

static_assert(source_location_of(annotations_of(^^S)[0]).line() ==
              source_location_of(^^S).line());

constexpr struct S {} s;

[[=s]] void fnWithS();
static_assert(type_of(annotations_of(^^fnWithS)[0]) == ^^S);
static_assert(type_of(value_of(annotations_of(^^fnWithS)[0])) == ^^S);
}  // namespace non_dependent

                                  // =========
                                  // dependent
                                  // =========

namespace dependent {
template <std::meta::info R>
  [[=[:value_of(annotations_of(R)[0]):]]] void TFn();
template <std::meta::info R>
  struct [[=[:value_of(annotations_of(R)[0]):]]] TCls {};

static_assert(extract<int>(annotations_of(^^TFn<^^non_dependent::fn>)[0]) == 1);
static_assert(extract<int>(annotations_of(^^TCls<^^non_dependent::S>)[0]) == 3);
}  // namespace dependent

                                 // ==========
                                 // comparison
                                 // ==========

namespace comparison {
struct [[=42, =42.0f]] S1;
struct [[=42  =40]] S2;

static_assert(annotations_of(^^S1)[0] == annotations_of(^^S1)[0]);
static_assert(annotations_of(^^S1)[0] != annotations_of(^^S2)[0]);
static_assert(annotations_of(^^S1)[0] != annotations_of(^^S1)[1]);

static_assert(value_of(annotations_of(^^S1)[0]) == std::meta::reflect_value(42));
static_assert(value_of(annotations_of(^^S1)[0]) ==
              value_of(annotations_of(^^S2)[0]));
}  // namespace comparison

                           // =======================
                           // conditional_annotations
                           // =======================

namespace conditional_annotations {
[[=1, =2, =3]] void fn();

template <std::meta::info R>
struct TCls {};

template <std::meta::info R> requires (is_function(R))
struct [[=int(annotations_of(R).size()), maybe_unused]] TCls<R> {};

static_assert(annotations_of(^^TCls<^^::>).size() == 0);
static_assert(annotation_of_type<int>(^^TCls<^^fn>) == 3);
}  // namespace conditional_annotations

                            // ====================
                            // annotation_injection
                            // ====================

namespace annotation_injection {
void fn();
template <std::meta::info R>
    [[=annotations_of(R)[0]]] consteval std::meta::info tfn(std::meta::info V) {
  return annotate(R, V);
}

static_assert(annotations_of(^^fn).size() == 0);

constexpr auto a1 = annotate(^^fn, std::meta::reflect_value(1));
static_assert(annotations_of(^^fn) == std::vector {a1});
static_assert(extract<int>(a1) == 1);

constexpr auto a2 = tfn<^^fn>(std::meta::reflect_value(2.0f));
static_assert(annotations_of(^^fn) == std::vector {a1, a2});
static_assert(annotation_of_type<float>(^^fn) == 2.0f);
}  // namespace annotation_injection

                       // ==============================
                       // accumulation_over_declarations
                       // ==============================

namespace accumulation_over_declarations {
void fn();
static_assert(annotations_of(^^fn).size() == 0);
[[=1, =2]] void fn();
static_assert(annotations_of(^^fn).size() == 2);
constexpr auto i1 = annotate(^^fn, std::meta::reflect_value(3));
static_assert(annotations_of(^^fn).size() == 3);
[[=4, =5]] void fn();
static_assert(annotations_of(^^fn).size() == 5);
constexpr auto i2 = annotate(^^fn, std::meta::reflect_value(6));
static_assert(annotations_of(^^fn).size() == 6);
void fn();
static_assert(annotations_of(^^fn).size() == 6);

constexpr auto idxOf = [](int v) consteval {
  auto annots = annotations_of(^^fn);
  for (size_t k = 0; k < annots.size(); ++k)
    if (extract<int>(annots[k]) == v)
      return k;

  std::unreachable();
};
constexpr auto p1 = idxOf(1), p4 = idxOf(4);

static_assert(extract<int>(annotations_of(^^fn)[p1]) == 1);
static_assert(extract<int>(annotations_of(^^fn)[p1 + 1]) == 2);
static_assert(annotations_of(^^fn)[p1 + 2] == i1);

static_assert(extract<int>(annotations_of(^^fn)[p4]) == 4);
static_assert(extract<int>(annotations_of(^^fn)[p4 + 1]) == 5);
static_assert(annotations_of(^^fn)[p4 + 2] == i2);
}  // namespace accumulation_over_declarations

                       // ===============================
                       // ledger_based_consteval_variable
                       // ===============================

namespace ledger_based_consteval_variable {
struct Counter {
private:
  static int ledger;

public:
  static consteval int next(int i = 1) {
    auto history = annotations_of(^^ledger);

    int entry = i;
    if (history.size() > 0)
      entry += extract<int>(history.back());

    annotate(^^ledger, std::meta::reflect_value(entry));
    return entry;
  }
};
constexpr auto c1 = Counter::next();
constexpr auto c2 = Counter::next();
constexpr auto c3 = Counter::next(-2);
constexpr auto c4 = Counter::next(5);

static_assert(c1 == 1);
static_assert(c2 == 2);
static_assert(c3 == 0);
static_assert(c4 == 5);
}  // namespace ledger_based_consteval_variable

int main() { }
