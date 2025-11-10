// RUN: %clang_cc1 -std=c++2b -fsyntax-only -verify %s

//------------------------------------------------------------------------------
// Basic well-formed forms
//------------------------------------------------------------------------------

// Single annotation with constant
[[= 42]] int a;           
[[=0]] int b;             
[[= (1+2)]] int c;        
// Multiple annotations in one specifier
[[=1, =2]] int d;         

// Annotations separated into multiple specifiers
[[=1]] [[=2]] int e;      


//------------------------------------------------------------------------------
// Ill-formed forms (syntax/structure)
//------------------------------------------------------------------------------

[[nodiscard = 42]] int f;     // expected-error {{expected ','}} expected-warning {{'nodiscard' attribute only applies to Objective-C methods, enums, structs, unions, classes, functions, function pointers, and typedefs}}

[[=42, nodiscard]] int h; 
[[nodiscard, = 42]] int i; // expected-warning {{'nodiscard' attribute only applies to Objective-C methods, enums, structs, unions, classes, functions, function pointers, and typedefs}}

// should it pass? I don't think so...
// [[= ]] int j;             

// Unterminated annotation list
// [[=42,]] int k;           

//------------------------------------------------------------------------------
// Edge & nested cases
//------------------------------------------------------------------------------

// Annotation in templated declaration
template <int N> [[= N]] void foo(); 

// Inside a class definition
struct S {
  [[= 1]] int x;          
  [[=2, =3]] int y;       
};

// Annotating a function
[[= 42]] void bar();      

[[= 7]] int baz(int);     

