; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=riscv32 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s -check-prefix=RV32
; RUN: llc -mtriple=riscv32 -global-merge-min-data-size=5 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s -check-prefix=RV32-MINSIZE

@ig1 = internal global i32 0, align 4
@ig2 = internal global i32 0, align 4

@eg1 = dso_local global i32 0, align 4
@eg2 = dso_local global i32 0, align 4

define void @f1(i32 %a) nounwind {
; RV32-LABEL: f1:
; RV32:       # %bb.0:
; RV32-NEXT:    lui a1, %hi(.L_MergedGlobals)
; RV32-NEXT:    sw a0, %lo(.L_MergedGlobals)(a1)
; RV32-NEXT:    addi a1, a1, %lo(.L_MergedGlobals)
; RV32-NEXT:    sw a0, 4(a1)
; RV32-NEXT:    sw a0, 8(a1)
; RV32-NEXT:    sw a0, 12(a1)
; RV32-NEXT:    ret
;
; RV32-MINSIZE-LABEL: f1:
; RV32-MINSIZE:       # %bb.0:
; RV32-MINSIZE-NEXT:    lui a1, %hi(ig1)
; RV32-MINSIZE-NEXT:    sw a0, %lo(ig1)(a1)
; RV32-MINSIZE-NEXT:    lui a1, %hi(ig2)
; RV32-MINSIZE-NEXT:    sw a0, %lo(ig2)(a1)
; RV32-MINSIZE-NEXT:    lui a1, %hi(eg1)
; RV32-MINSIZE-NEXT:    sw a0, %lo(eg1)(a1)
; RV32-MINSIZE-NEXT:    lui a1, %hi(eg2)
; RV32-MINSIZE-NEXT:    sw a0, %lo(eg2)(a1)
; RV32-MINSIZE-NEXT:    ret
  store i32 %a, ptr @ig1, align 4
  store i32 %a, ptr @ig2, align 4
  store i32 %a, ptr @eg1, align 4
  store i32 %a, ptr @eg2, align 4
  ret void
}
