// RUN: %clang_cc1 %s -std=c++23 -verify

enum class [[clang::checked_enum]] Opaque : unsigned int; // expected-warning {{'checked_enum' attribute only applies to enum definitions}}

enum class [[clang::checked_enum]] Levels : unsigned int {
    Debug = 0,
    Info = 12,
    Warn = 120,
    Critical = 500
};

constexpr Levels Good = static_cast<Levels>(12); // known valid, no diagnostic

constexpr Levels Bad = static_cast<Levels>(5); // expected-error {{value 5 is not an enumerator of checked enumeration 'Levels'}}

Levels convert(unsigned x) {
    return static_cast<Levels>(x); // not a constant, checked at runtime instead
}

void f() {
    Levels i = static_cast<Levels>(12); // known valid, no diagnostic

    Levels j = static_cast<Levels>(5); // expected-error {{value 5 is not an enumerator of checked enumeration 'Levels'}}
}
