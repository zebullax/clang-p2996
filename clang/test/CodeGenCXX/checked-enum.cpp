// RUN: %clang_cc1 %s -std=c++23 -fsanitize=enum-checked-cast -emit-llvm -o - | FileCheck %s

// A static_cast to a checked enum whose value cannot be proven valid at
// compile time (e.g. a cast of a non-constant argument) lowers to a runtime
// check against every enumerator, calling into the UBSan runtime when none
// match. Without -fsanitize=enum-checked-cast no check is emitted at all;
// see the no-sanitize RUN line below.

enum class [[clang::checked_enum]] Levels : unsigned int {
    Debug = 0,
    Info = 12,
    Warn = 120,
    Critical = 500
};

// CHECK-LABEL: define{{.*}} i32 @_Z7convertj(i32 noundef %x)
Levels convert(unsigned x) {
    // CHECK: icmp eq i32 0, %0
    // CHECK: icmp eq i32 12, %0
    // CHECK: icmp eq i32 120, %0
    // CHECK: icmp eq i32 500, %0
    // CHECK: br i1 {{.*}}, label %[[CONT:[^,]+]], label %[[HANDLER:[^,]+]],{{.*}}!nosanitize

    // CHECK: [[HANDLER]]:
    // CHECK: call void @__ubsan_handle_enum_checked_cast_abort(
    // CHECK-NEXT: unreachable

    // CHECK: [[CONT]]:
    // CHECK-NEXT: ret i32 %0
    return static_cast<Levels>(x);
}

// RUN: %clang_cc1 %s -std=c++23 -emit-llvm -o - | FileCheck %s --check-prefix=NOSAN

// NOSAN-LABEL: define{{.*}} i32 @_Z7convertj(i32 noundef %x)
// NOSAN-NOT: icmp
// NOSAN-NOT: __ubsan_handle_enum_checked_cast
