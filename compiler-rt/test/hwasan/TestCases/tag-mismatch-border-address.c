// Make sure we do not segfault when checking an address close to kLowMemEnd.
// RUN: %clang_hwasan  %s -o %t
// RUN: not %run %t 2>&1 | FileCheck %s --check-prefix=CHECK

// REQUIRES: aarch64-target-arch || riscv64-target-arch

#include <sanitizer/hwasan_interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <sys/mman.h>

static volatile char sink;
extern void *__hwasan_shadow_memory_dynamic_address;

int main(int argc, char **argv) {
  size_t page_size = getauxval(AT_PAGESZ);
  void *high_addr = (char *)__hwasan_shadow_memory_dynamic_address - page_size;
  void *r = mmap(high_addr, page_size, PROT_READ,
                 MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
  if (r == MAP_FAILED) {
    fprintf(stderr, "Failed to mmap\n");
    abort();
  }
  volatile char *x = (char *)__hwasan_tag_pointer(r, 4);
  sink = *x;
}

// CHECK-NOT: Failed to mmap
// CHECK-NOT: Segmentation fault
// CHECK-NOT: SEGV
