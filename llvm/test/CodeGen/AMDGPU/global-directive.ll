; RUN: llc -mtriple=amdgcn < %s | FileCheck -check-prefix=SI %s
; RUN: llc -mtriple=amdgcn -mcpu=tonga < %s | FileCheck -check-prefix=SI %s

; Make sure the GlobalDirective isn't merged with the function name

; SI:	.globl	foo
; SI: {{^}}foo:
define amdgpu_kernel void @foo(ptr addrspace(1) %out, ptr addrspace(1) %in) {
  %b_ptr = getelementptr i32, ptr addrspace(1) %in, i32 1
  %a = load i32, ptr addrspace(1) %in
  %b = load i32, ptr addrspace(1) %b_ptr
  %result = add i32 %a, %b
  store i32 %result, ptr addrspace(1) %out
  ret void
}
