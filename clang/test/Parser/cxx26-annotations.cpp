// RUN: %clang_cc1 -std=c++2b -fsyntax-only -verify %s

int arr[2];
struct S{int a; int b;}; 
S arr2[2];

//------------------------------------------------------------------------------
// Basic well-formed forms
//------------------------------------------------------------------------------

// Single annotation with constant
[[=42]] int a;
[[=0]] int b;
[[= (1+2)]] int c;

// Multiple annotations in one specifier
[[=1, =2]] int d;

// Annotations separated into multiple specifiers
[[=1]] [[=2]] int e;

auto a_lambda = [] [[=1]] () {};
[[=1]];                   
void parser_test_statements() {
  [[=1]] {}                   
  [[=1]] if (true) {}         
  [[=1]] while (false) {}
  [[=1]] goto lab; 
  [[=1]] lab:;            
  if ([[=1]] int x = 0) {}     
  switch (0) {
    [[=1]] case 1:
    [[=1]] break;
    [[=1]] default:
      break;
  }
  for ([[=1]] auto a : arr) {}
  for ([[=1]] auto [a1, a2] : arr2) {} 
  [[=1]] asm ("");
}
[[=1]] inline void good_baz_inline () {}
inline [[=1]] void bad_baz_inline () {} //expected-error {{an attribute list cannot appear here}}
constexpr [[=1]] int qux_constexpr () { return 0; } //expected-error {{an attribute list cannot appear here}}
[[=1]] int d_var;
int const [[=1]] e_const = 1;
struct [[=1]] C_struct {};

//------------------------------------------------------------------------------
// Ill-formed forms (syntax/structure)
//------------------------------------------------------------------------------

// Mixing attributes and annotations (parser-level errors)
[[nodiscard = 42]] int f;  //expected-warning {{'nodiscard' attribute only applies to Objective-C methods, enums, structs, unions, classes, functions, function pointers, and typedefs}} expected-error {{expected ','}}
[[=42, nodiscard]] int h;  
[[nodiscard, =42]] int i;  //expected-warning {{'nodiscard' attribute only applies to Objective-C methods, enums, structs, unions, classes, functions, function pointers, and typedefs}}

//Enums
enum class [[=1]] E1 {A, B};
enum class A1 {
  A [[=1]],
  B
};
[[=1]] enum class E2 {A, B, C}; //expected-error {{misplaced attributes; expected attributes here}}
[[=2, =3]] enum class E3 {D, E, F}; //expected-error {{misplaced attributes; expected attributes here}}
enum class E4 {
  [[=1]] G,  //expected-error {{expected identifier}}
  H,
  [[=5]] I  //expected-error {{expected identifier}}
};

// this passes in Parser, but it should probably fail in Sema
[[=]]int j;
[[=42,]] int k;