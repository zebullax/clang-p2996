// RUN: %clang_cc1 %s -std=c++23 -emit-llvm -o - | FileCheck %s

// A static_cast to a checked enum whose value cannot be proven valid at
// compile time (e.g. a cast of a non-constant argument) lowers to a runtime
// check against every enumerator, trapping when none match.

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
    // CHECK: br i1 {{.*}}, label %[[VALID:.*]], label %[[INVALID:.*]]

    // CHECK: [[VALID]]:
    // CHECK-NEXT: ret i32 %0

    // CHECK: [[INVALID]]:
    // CHECK-NEXT: call void @llvm.trap()
    // CHECK-NEXT: unreachable
    return static_cast<Levels>(x);
}
