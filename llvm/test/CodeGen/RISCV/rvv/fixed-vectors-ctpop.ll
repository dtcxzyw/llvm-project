; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=riscv32 -mattr=+m,+v -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,RV32
; RUN: llc -mtriple=riscv64 -mattr=+m,+v -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,RV64
; RUN: llc -mtriple=riscv32 -mattr=+v,+zvbb -verify-machineinstrs < %s | FileCheck %s --check-prefixes=ZVBB
; RUN: llc -mtriple=riscv64 -mattr=+v,+zvbb -verify-machineinstrs < %s | FileCheck %s --check-prefixes=ZVBB

define void @ctpop_v16i8(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v16i8:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 16, e8, m1, ta, ma
; CHECK-NEXT:    vle8.v v8, (a0)
; CHECK-NEXT:    li a1, 85
; CHECK-NEXT:    vsrl.vi v9, v8, 1
; CHECK-NEXT:    vand.vx v9, v9, a1
; CHECK-NEXT:    li a1, 51
; CHECK-NEXT:    vsub.vv v8, v8, v9
; CHECK-NEXT:    vand.vx v9, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    vadd.vv v8, v9, v8
; CHECK-NEXT:    vsrl.vi v9, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v9
; CHECK-NEXT:    vand.vi v8, v8, 15
; CHECK-NEXT:    vse8.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v16i8:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 16, e8, m1, ta, ma
; ZVBB-NEXT:    vle8.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse8.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <16 x i8>, ptr %x
  %b = load <16 x i8>, ptr %y
  %c = call <16 x i8> @llvm.ctpop.v16i8(<16 x i8> %a)
  store <16 x i8> %c, ptr %x
  ret void
}
declare <16 x i8> @llvm.ctpop.v16i8(<16 x i8>)

define void @ctpop_v8i16(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i16:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e16, m1, ta, ma
; CHECK-NEXT:    vle16.v v8, (a0)
; CHECK-NEXT:    lui a1, 5
; CHECK-NEXT:    addi a1, a1, 1365
; CHECK-NEXT:    vsrl.vi v9, v8, 1
; CHECK-NEXT:    vand.vx v9, v9, a1
; CHECK-NEXT:    lui a1, 3
; CHECK-NEXT:    addi a1, a1, 819
; CHECK-NEXT:    vsub.vv v8, v8, v9
; CHECK-NEXT:    vand.vx v9, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 1
; CHECK-NEXT:    addi a1, a1, -241
; CHECK-NEXT:    vadd.vv v8, v9, v8
; CHECK-NEXT:    vsrl.vi v9, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v9
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    li a1, 257
; CHECK-NEXT:    vmul.vx v8, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 8
; CHECK-NEXT:    vse16.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i16:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e16, m1, ta, ma
; ZVBB-NEXT:    vle16.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse16.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <8 x i16>, ptr %x
  %b = load <8 x i16>, ptr %y
  %c = call <8 x i16> @llvm.ctpop.v8i16(<8 x i16> %a)
  store <8 x i16> %c, ptr %x
  ret void
}
declare <8 x i16> @llvm.ctpop.v8i16(<8 x i16>)

define void @ctpop_v4i32(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v4i32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    lui a1, 349525
; CHECK-NEXT:    addi a1, a1, 1365
; CHECK-NEXT:    vsrl.vi v9, v8, 1
; CHECK-NEXT:    vand.vx v9, v9, a1
; CHECK-NEXT:    lui a1, 209715
; CHECK-NEXT:    addi a1, a1, 819
; CHECK-NEXT:    vsub.vv v8, v8, v9
; CHECK-NEXT:    vand.vx v9, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 61681
; CHECK-NEXT:    addi a1, a1, -241
; CHECK-NEXT:    vadd.vv v8, v9, v8
; CHECK-NEXT:    vsrl.vi v9, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v9
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 4112
; CHECK-NEXT:    addi a1, a1, 257
; CHECK-NEXT:    vmul.vx v8, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 24
; CHECK-NEXT:    vse32.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i32:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse32.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <4 x i32>, ptr %x
  %b = load <4 x i32>, ptr %y
  %c = call <4 x i32> @llvm.ctpop.v4i32(<4 x i32> %a)
  store <4 x i32> %c, ptr %x
  ret void
}
declare <4 x i32> @llvm.ctpop.v4i32(<4 x i32>)

define void @ctpop_v2i64(ptr %x, ptr %y) {
; RV32-LABEL: ctpop_v2i64:
; RV32:       # %bb.0:
; RV32-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV32-NEXT:    vle64.v v8, (a0)
; RV32-NEXT:    lui a1, 349525
; RV32-NEXT:    addi a1, a1, 1365
; RV32-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; RV32-NEXT:    vmv.v.x v9, a1
; RV32-NEXT:    lui a1, 209715
; RV32-NEXT:    addi a1, a1, 819
; RV32-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV32-NEXT:    vsrl.vi v10, v8, 1
; RV32-NEXT:    vand.vv v9, v10, v9
; RV32-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; RV32-NEXT:    vmv.v.x v10, a1
; RV32-NEXT:    lui a1, 61681
; RV32-NEXT:    addi a1, a1, -241
; RV32-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV32-NEXT:    vsub.vv v8, v8, v9
; RV32-NEXT:    vand.vv v9, v8, v10
; RV32-NEXT:    vsrl.vi v8, v8, 2
; RV32-NEXT:    vand.vv v8, v8, v10
; RV32-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; RV32-NEXT:    vmv.v.x v10, a1
; RV32-NEXT:    lui a1, 4112
; RV32-NEXT:    addi a1, a1, 257
; RV32-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV32-NEXT:    vadd.vv v8, v9, v8
; RV32-NEXT:    vsrl.vi v9, v8, 4
; RV32-NEXT:    vadd.vv v8, v8, v9
; RV32-NEXT:    vsetivli zero, 4, e32, m1, ta, ma
; RV32-NEXT:    vmv.v.x v9, a1
; RV32-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV32-NEXT:    vand.vv v8, v8, v10
; RV32-NEXT:    vmul.vv v8, v8, v9
; RV32-NEXT:    li a1, 56
; RV32-NEXT:    vsrl.vx v8, v8, a1
; RV32-NEXT:    vse64.v v8, (a0)
; RV32-NEXT:    ret
;
; RV64-LABEL: ctpop_v2i64:
; RV64:       # %bb.0:
; RV64-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; RV64-NEXT:    vle64.v v8, (a0)
; RV64-NEXT:    lui a1, 349525
; RV64-NEXT:    lui a2, 209715
; RV64-NEXT:    lui a3, 61681
; RV64-NEXT:    lui a4, 4112
; RV64-NEXT:    addi a1, a1, 1365
; RV64-NEXT:    addi a2, a2, 819
; RV64-NEXT:    addi a3, a3, -241
; RV64-NEXT:    addi a4, a4, 257
; RV64-NEXT:    slli a5, a1, 32
; RV64-NEXT:    add a1, a1, a5
; RV64-NEXT:    slli a5, a2, 32
; RV64-NEXT:    add a2, a2, a5
; RV64-NEXT:    slli a5, a3, 32
; RV64-NEXT:    add a3, a3, a5
; RV64-NEXT:    slli a5, a4, 32
; RV64-NEXT:    add a4, a4, a5
; RV64-NEXT:    vsrl.vi v9, v8, 1
; RV64-NEXT:    vand.vx v9, v9, a1
; RV64-NEXT:    vsub.vv v8, v8, v9
; RV64-NEXT:    vand.vx v9, v8, a2
; RV64-NEXT:    vsrl.vi v8, v8, 2
; RV64-NEXT:    vand.vx v8, v8, a2
; RV64-NEXT:    vadd.vv v8, v9, v8
; RV64-NEXT:    vsrl.vi v9, v8, 4
; RV64-NEXT:    vadd.vv v8, v8, v9
; RV64-NEXT:    vand.vx v8, v8, a3
; RV64-NEXT:    vmul.vx v8, v8, a4
; RV64-NEXT:    li a1, 56
; RV64-NEXT:    vsrl.vx v8, v8, a1
; RV64-NEXT:    vse64.v v8, (a0)
; RV64-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v2i64:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 2, e64, m1, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse64.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <2 x i64>, ptr %x
  %b = load <2 x i64>, ptr %y
  %c = call <2 x i64> @llvm.ctpop.v2i64(<2 x i64> %a)
  store <2 x i64> %c, ptr %x
  ret void
}
declare <2 x i64> @llvm.ctpop.v2i64(<2 x i64>)

define void @ctpop_v32i8(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v32i8:
; CHECK:       # %bb.0:
; CHECK-NEXT:    li a1, 32
; CHECK-NEXT:    vsetvli zero, a1, e8, m2, ta, ma
; CHECK-NEXT:    vle8.v v8, (a0)
; CHECK-NEXT:    li a1, 85
; CHECK-NEXT:    vsrl.vi v10, v8, 1
; CHECK-NEXT:    vand.vx v10, v10, a1
; CHECK-NEXT:    li a1, 51
; CHECK-NEXT:    vsub.vv v8, v8, v10
; CHECK-NEXT:    vand.vx v10, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    vadd.vv v8, v10, v8
; CHECK-NEXT:    vsrl.vi v10, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v10
; CHECK-NEXT:    vand.vi v8, v8, 15
; CHECK-NEXT:    vse8.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v32i8:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    li a1, 32
; ZVBB-NEXT:    vsetvli zero, a1, e8, m2, ta, ma
; ZVBB-NEXT:    vle8.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse8.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <32 x i8>, ptr %x
  %b = load <32 x i8>, ptr %y
  %c = call <32 x i8> @llvm.ctpop.v32i8(<32 x i8> %a)
  store <32 x i8> %c, ptr %x
  ret void
}
declare <32 x i8> @llvm.ctpop.v32i8(<32 x i8>)

define void @ctpop_v16i16(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v16i16:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 16, e16, m2, ta, ma
; CHECK-NEXT:    vle16.v v8, (a0)
; CHECK-NEXT:    lui a1, 5
; CHECK-NEXT:    addi a1, a1, 1365
; CHECK-NEXT:    vsrl.vi v10, v8, 1
; CHECK-NEXT:    vand.vx v10, v10, a1
; CHECK-NEXT:    lui a1, 3
; CHECK-NEXT:    addi a1, a1, 819
; CHECK-NEXT:    vsub.vv v8, v8, v10
; CHECK-NEXT:    vand.vx v10, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 1
; CHECK-NEXT:    addi a1, a1, -241
; CHECK-NEXT:    vadd.vv v8, v10, v8
; CHECK-NEXT:    vsrl.vi v10, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v10
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    li a1, 257
; CHECK-NEXT:    vmul.vx v8, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 8
; CHECK-NEXT:    vse16.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v16i16:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 16, e16, m2, ta, ma
; ZVBB-NEXT:    vle16.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse16.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <16 x i16>, ptr %x
  %b = load <16 x i16>, ptr %y
  %c = call <16 x i16> @llvm.ctpop.v16i16(<16 x i16> %a)
  store <16 x i16> %c, ptr %x
  ret void
}
declare <16 x i16> @llvm.ctpop.v16i16(<16 x i16>)

define void @ctpop_v8i32(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    lui a1, 349525
; CHECK-NEXT:    addi a1, a1, 1365
; CHECK-NEXT:    vsrl.vi v10, v8, 1
; CHECK-NEXT:    vand.vx v10, v10, a1
; CHECK-NEXT:    lui a1, 209715
; CHECK-NEXT:    addi a1, a1, 819
; CHECK-NEXT:    vsub.vv v8, v8, v10
; CHECK-NEXT:    vand.vx v10, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 2
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 61681
; CHECK-NEXT:    addi a1, a1, -241
; CHECK-NEXT:    vadd.vv v8, v10, v8
; CHECK-NEXT:    vsrl.vi v10, v8, 4
; CHECK-NEXT:    vadd.vv v8, v8, v10
; CHECK-NEXT:    vand.vx v8, v8, a1
; CHECK-NEXT:    lui a1, 4112
; CHECK-NEXT:    addi a1, a1, 257
; CHECK-NEXT:    vmul.vx v8, v8, a1
; CHECK-NEXT:    vsrl.vi v8, v8, 24
; CHECK-NEXT:    vse32.v v8, (a0)
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i32:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse32.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <8 x i32>, ptr %x
  %b = load <8 x i32>, ptr %y
  %c = call <8 x i32> @llvm.ctpop.v8i32(<8 x i32> %a)
  store <8 x i32> %c, ptr %x
  ret void
}
define <8 x i1> @ctpop_v8i32_ult_two(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i32_ult_two:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vand.vv v8, v8, v10
; CHECK-NEXT:    vmseq.vi v0, v8, 0
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i32_ult_two:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsleu.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <8 x i32>, ptr %x
  %b = load <8 x i32>, ptr %y
  %c = call <8 x i32> @llvm.ctpop.v8i32(<8 x i32> %a)
  %cmp = icmp ult <8 x i32> %c, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  ret <8 x i1> %cmp
}
define <8 x i1> @ctpop_v8i32_ugt_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i32_ugt_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vand.vv v8, v8, v10
; CHECK-NEXT:    vmsne.vi v0, v8, 0
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i32_ugt_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsgtu.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <8 x i32>, ptr %x
  %b = load <8 x i32>, ptr %y
  %c = call <8 x i32> @llvm.ctpop.v8i32(<8 x i32> %a)
  %cmp = icmp ugt <8 x i32> %c, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret <8 x i1> %cmp
}
define <8 x i1> @ctpop_v8i32_eq_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i32_eq_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vxor.vv v8, v8, v10
; CHECK-NEXT:    vmsltu.vv v0, v10, v8
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i32_eq_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmseq.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <8 x i32>, ptr %x
  %b = load <8 x i32>, ptr %y
  %c = call <8 x i32> @llvm.ctpop.v8i32(<8 x i32> %a)
  %cmp = icmp eq <8 x i32> %c, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret <8 x i1> %cmp
}
define <8 x i1> @ctpop_v8i32_ne_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v8i32_ne_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; CHECK-NEXT:    vle32.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vxor.vv v8, v8, v10
; CHECK-NEXT:    vmsleu.vv v0, v8, v10
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v8i32_ne_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; ZVBB-NEXT:    vle32.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsne.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <8 x i32>, ptr %x
  %b = load <8 x i32>, ptr %y
  %c = call <8 x i32> @llvm.ctpop.v8i32(<8 x i32> %a)
  %cmp = icmp ne <8 x i32> %c, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret <8 x i1> %cmp
}
declare <8 x i32> @llvm.ctpop.v8i32(<8 x i32>)

define void @ctpop_v4i64(ptr %x, ptr %y) {
; RV32-LABEL: ctpop_v4i64:
; RV32:       # %bb.0:
; RV32-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV32-NEXT:    vle64.v v8, (a0)
; RV32-NEXT:    lui a1, 349525
; RV32-NEXT:    addi a1, a1, 1365
; RV32-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; RV32-NEXT:    vmv.v.x v10, a1
; RV32-NEXT:    lui a1, 209715
; RV32-NEXT:    addi a1, a1, 819
; RV32-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV32-NEXT:    vsrl.vi v12, v8, 1
; RV32-NEXT:    vand.vv v10, v12, v10
; RV32-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; RV32-NEXT:    vmv.v.x v12, a1
; RV32-NEXT:    lui a1, 61681
; RV32-NEXT:    addi a1, a1, -241
; RV32-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV32-NEXT:    vsub.vv v8, v8, v10
; RV32-NEXT:    vand.vv v10, v8, v12
; RV32-NEXT:    vsrl.vi v8, v8, 2
; RV32-NEXT:    vand.vv v8, v8, v12
; RV32-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; RV32-NEXT:    vmv.v.x v12, a1
; RV32-NEXT:    lui a1, 4112
; RV32-NEXT:    addi a1, a1, 257
; RV32-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV32-NEXT:    vadd.vv v8, v10, v8
; RV32-NEXT:    vsrl.vi v10, v8, 4
; RV32-NEXT:    vadd.vv v8, v8, v10
; RV32-NEXT:    vsetivli zero, 8, e32, m2, ta, ma
; RV32-NEXT:    vmv.v.x v10, a1
; RV32-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV32-NEXT:    vand.vv v8, v8, v12
; RV32-NEXT:    vmul.vv v8, v8, v10
; RV32-NEXT:    li a1, 56
; RV32-NEXT:    vsrl.vx v8, v8, a1
; RV32-NEXT:    vse64.v v8, (a0)
; RV32-NEXT:    ret
;
; RV64-LABEL: ctpop_v4i64:
; RV64:       # %bb.0:
; RV64-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; RV64-NEXT:    vle64.v v8, (a0)
; RV64-NEXT:    lui a1, 349525
; RV64-NEXT:    lui a2, 209715
; RV64-NEXT:    lui a3, 61681
; RV64-NEXT:    lui a4, 4112
; RV64-NEXT:    addi a1, a1, 1365
; RV64-NEXT:    addi a2, a2, 819
; RV64-NEXT:    addi a3, a3, -241
; RV64-NEXT:    addi a4, a4, 257
; RV64-NEXT:    slli a5, a1, 32
; RV64-NEXT:    add a1, a1, a5
; RV64-NEXT:    slli a5, a2, 32
; RV64-NEXT:    add a2, a2, a5
; RV64-NEXT:    slli a5, a3, 32
; RV64-NEXT:    add a3, a3, a5
; RV64-NEXT:    slli a5, a4, 32
; RV64-NEXT:    add a4, a4, a5
; RV64-NEXT:    vsrl.vi v10, v8, 1
; RV64-NEXT:    vand.vx v10, v10, a1
; RV64-NEXT:    vsub.vv v8, v8, v10
; RV64-NEXT:    vand.vx v10, v8, a2
; RV64-NEXT:    vsrl.vi v8, v8, 2
; RV64-NEXT:    vand.vx v8, v8, a2
; RV64-NEXT:    vadd.vv v8, v10, v8
; RV64-NEXT:    vsrl.vi v10, v8, 4
; RV64-NEXT:    vadd.vv v8, v8, v10
; RV64-NEXT:    vand.vx v8, v8, a3
; RV64-NEXT:    vmul.vx v8, v8, a4
; RV64-NEXT:    li a1, 56
; RV64-NEXT:    vsrl.vx v8, v8, a1
; RV64-NEXT:    vse64.v v8, (a0)
; RV64-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i64:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vse64.v v8, (a0)
; ZVBB-NEXT:    ret
  %a = load <4 x i64>, ptr %x
  %b = load <4 x i64>, ptr %y
  %c = call <4 x i64> @llvm.ctpop.v4i64(<4 x i64> %a)
  store <4 x i64> %c, ptr %x
  ret void
}
define <4 x i1> @ctpop_v4i64_ult_two(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v4i64_ult_two:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; CHECK-NEXT:    vle64.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vand.vv v8, v8, v10
; CHECK-NEXT:    vmseq.vi v0, v8, 0
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i64_ult_two:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsleu.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <4 x i64>, ptr %x
  %b = load <4 x i64>, ptr %y
  %c = call <4 x i64> @llvm.ctpop.v4i64(<4 x i64> %a)
  %cmp = icmp ult <4 x i64> %c, <i64 2, i64 2, i64 2, i64 2>
  ret <4 x i1> %cmp
}
define <4 x i1> @ctpop_v4i64_ugt_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v4i64_ugt_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; CHECK-NEXT:    vle64.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vand.vv v8, v8, v10
; CHECK-NEXT:    vmsne.vi v0, v8, 0
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i64_ugt_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsgtu.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <4 x i64>, ptr %x
  %b = load <4 x i64>, ptr %y
  %c = call <4 x i64> @llvm.ctpop.v4i64(<4 x i64> %a)
  %cmp = icmp ugt <4 x i64> %c, <i64 1, i64 1, i64 1, i64 1>
  ret <4 x i1> %cmp
}
define <4 x i1> @ctpop_v4i64_eq_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v4i64_eq_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; CHECK-NEXT:    vle64.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vxor.vv v8, v8, v10
; CHECK-NEXT:    vmsltu.vv v0, v10, v8
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i64_eq_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmseq.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <4 x i64>, ptr %x
  %b = load <4 x i64>, ptr %y
  %c = call <4 x i64> @llvm.ctpop.v4i64(<4 x i64> %a)
  %cmp = icmp eq <4 x i64> %c, <i64 1, i64 1, i64 1, i64 1>
  ret <4 x i1> %cmp
}
define <4 x i1> @ctpop_v4i64_ne_one(ptr %x, ptr %y) {
; CHECK-LABEL: ctpop_v4i64_ne_one:
; CHECK:       # %bb.0:
; CHECK-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; CHECK-NEXT:    vle64.v v8, (a0)
; CHECK-NEXT:    vadd.vi v10, v8, -1
; CHECK-NEXT:    vxor.vv v8, v8, v10
; CHECK-NEXT:    vmsleu.vv v0, v8, v10
; CHECK-NEXT:    ret
;
; ZVBB-LABEL: ctpop_v4i64_ne_one:
; ZVBB:       # %bb.0:
; ZVBB-NEXT:    vsetivli zero, 4, e64, m2, ta, ma
; ZVBB-NEXT:    vle64.v v8, (a0)
; ZVBB-NEXT:    vcpop.v v8, v8
; ZVBB-NEXT:    vmsne.vi v0, v8, 1
; ZVBB-NEXT:    ret
  %a = load <4 x i64>, ptr %x
  %b = load <4 x i64>, ptr %y
  %c = call <4 x i64> @llvm.ctpop.v4i64(<4 x i64> %a)
  %cmp = icmp ne <4 x i64> %c, <i64 1, i64 1, i64 1, i64 1>
  ret <4 x i1> %cmp
}
declare <4 x i64> @llvm.ctpop.v4i64(<4 x i64>)
