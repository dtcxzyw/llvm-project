/// Same as global-location.cpp, but without debuginfo. In a separate file to
/// allow this test to also run on Windows (which can't be done for the
/// debuginfo variant).

// RUN: %clangxx_asan -O2 %S/global-location.cpp -o %t %if target={{.*-windows-msvc.*}} %{ -Wl,/DEBUG:NONE %} %else %{ -Wl,-S %} %if target={{.*-solaris.*}} %{ -fuse-ld= %}
// RUN: not %run %t g 2>&1 | FileCheck %s --check-prefix=CHECK --check-prefix=GLOB-NO-G
// RUN: not %run %t c 2>&1 | FileCheck %s --check-prefix=CHECK --check-prefix=CLASS_STATIC-NO-G
// RUN: not %run %t f 2>&1 | FileCheck %s --check-prefix=CHECK --check-prefix=FUNC_STATIC-NO-G
// RUN: not %run %t l 2>&1 | FileCheck %s --check-prefix=CHECK --check-prefix=LITERAL-NO-G

/// Solaris ld -S has different semantics, so enforce -fuse-ld= for
/// configurations that default to GNU ld.
// XFAIL: target={{.*solaris.*}}
// XFAIL: msvc
// CHECK: AddressSanitizer: global-buffer-overflow
// CLASS_STATIC-NO-G: 0x{{.*}} is located 4 bytes after global variable '{{.*}}C::array{{.*}}' defined in '{{.*}}global-location.cpp' {{.*}} of size 40
// GLOB-NO-G: 0x{{.*}} is located 4 bytes after global variable '{{.*}}global{{.*}}' defined in '{{.*}}global-location.cpp' {{.*}} of size 40
// FUNC_STATIC-NO-G: 0x{{.*}} is located 4 bytes after global variable '{{.*}}main{{.*}}::array{{.*}}' defined in '{{.*}}global-location.cpp' {{.*}} of size 40
// LITERAL-NO-G: 0x{{.*}} is located 0 bytes after global variable {{.*}} defined in '{{.*}}global-location.cpp' {{.*}} of size 11
// CHECK: SUMMARY: AddressSanitizer: global-buffer-overflow
