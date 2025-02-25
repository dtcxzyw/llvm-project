; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=aarch64-linux-gnu | FileCheck %s

define i32 @func(i32 %x, i32 %y) nounwind {
; CHECK-LABEL: func:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umull x8, w0, w1
; CHECK-NEXT:    lsr x9, x8, #32
; CHECK-NEXT:    extr w0, w9, w8, #2
; CHECK-NEXT:    ret
  %tmp = call i32 @llvm.umul.fix.i32(i32 %x, i32 %y, i32 2)
  ret i32 %tmp
}

define i64 @func2(i64 %x, i64 %y) nounwind {
; CHECK-LABEL: func2:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul x8, x0, x1
; CHECK-NEXT:    umulh x9, x0, x1
; CHECK-NEXT:    extr x0, x9, x8, #2
; CHECK-NEXT:    ret
  %tmp = call i64 @llvm.umul.fix.i64(i64 %x, i64 %y, i32 2)
  ret i64 %tmp
}

define i4 @func3(i4 %x, i4 %y) nounwind {
; CHECK-LABEL: func3:
; CHECK:       // %bb.0:
; CHECK-NEXT:    and w8, w1, #0xf
; CHECK-NEXT:    and w9, w0, #0xf
; CHECK-NEXT:    mul w8, w9, w8
; CHECK-NEXT:    lsr w0, w8, #2
; CHECK-NEXT:    ret
  %tmp = call i4 @llvm.umul.fix.i4(i4 %x, i4 %y, i32 2)
  ret i4 %tmp
}

;; These result in regular integer multiplication
define i32 @func4(i32 %x, i32 %y) nounwind {
; CHECK-LABEL: func4:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul w0, w0, w1
; CHECK-NEXT:    ret
  %tmp = call i32 @llvm.umul.fix.i32(i32 %x, i32 %y, i32 0)
  ret i32 %tmp
}

define i64 @func5(i64 %x, i64 %y) nounwind {
; CHECK-LABEL: func5:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul x0, x0, x1
; CHECK-NEXT:    ret
  %tmp = call i64 @llvm.umul.fix.i64(i64 %x, i64 %y, i32 0)
  ret i64 %tmp
}

define i4 @func6(i4 %x, i4 %y) nounwind {
; CHECK-LABEL: func6:
; CHECK:       // %bb.0:
; CHECK-NEXT:    and w8, w1, #0xf
; CHECK-NEXT:    and w9, w0, #0xf
; CHECK-NEXT:    mul w0, w9, w8
; CHECK-NEXT:    ret
  %tmp = call i4 @llvm.umul.fix.i4(i4 %x, i4 %y, i32 0)
  ret i4 %tmp
}

define i64 @func7(i64 %x, i64 %y) nounwind {
; CHECK-LABEL: func7:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul x8, x0, x1
; CHECK-NEXT:    umulh x9, x0, x1
; CHECK-NEXT:    extr x0, x9, x8, #32
; CHECK-NEXT:    ret
  %tmp = call i64 @llvm.umul.fix.i64(i64 %x, i64 %y, i32 32)
  ret i64 %tmp
}

define i64 @func8(i64 %x, i64 %y) nounwind {
; CHECK-LABEL: func8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul x8, x0, x1
; CHECK-NEXT:    umulh x9, x0, x1
; CHECK-NEXT:    extr x0, x9, x8, #63
; CHECK-NEXT:    ret
  %tmp = call i64 @llvm.umul.fix.i64(i64 %x, i64 %y, i32 63)
  ret i64 %tmp
}

define i64 @func9(i64 %x, i64 %y) nounwind {
; CHECK-LABEL: func9:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umulh x0, x0, x1
; CHECK-NEXT:    ret
  %tmp = call i64 @llvm.umul.fix.i64(i64 %x, i64 %y, i32 64)
  ret i64 %tmp
}

define <2 x i32> @vec(<2 x i32> %x, <2 x i32> %y) nounwind {
; CHECK-LABEL: vec:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul v0.2s, v0.2s, v1.2s
; CHECK-NEXT:    ret
  %tmp = call <2 x i32> @llvm.umul.fix.v2i32(<2 x i32> %x, <2 x i32> %y, i32 0)
  ret <2 x i32> %tmp
}

define <4 x i32> @vec2(<4 x i32> %x, <4 x i32> %y) nounwind {
; CHECK-LABEL: vec2:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mul v0.4s, v0.4s, v1.4s
; CHECK-NEXT:    ret
  %tmp = call <4 x i32> @llvm.umul.fix.v4i32(<4 x i32> %x, <4 x i32> %y, i32 0)
  ret <4 x i32> %tmp
}

define <4 x i64> @vec3(<4 x i64> %x, <4 x i64> %y) nounwind {
; CHECK-LABEL: vec3:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov x8, v2.d[1]
; CHECK-NEXT:    mov x9, v0.d[1]
; CHECK-NEXT:    fmov x10, d2
; CHECK-NEXT:    fmov x11, d0
; CHECK-NEXT:    mov x14, v3.d[1]
; CHECK-NEXT:    mov x15, v1.d[1]
; CHECK-NEXT:    mul x12, x11, x10
; CHECK-NEXT:    mul x13, x9, x8
; CHECK-NEXT:    umulh x8, x9, x8
; CHECK-NEXT:    umulh x9, x11, x10
; CHECK-NEXT:    fmov x10, d3
; CHECK-NEXT:    fmov x11, d1
; CHECK-NEXT:    mul x16, x11, x10
; CHECK-NEXT:    extr x8, x8, x13, #32
; CHECK-NEXT:    umulh x10, x11, x10
; CHECK-NEXT:    extr x9, x9, x12, #32
; CHECK-NEXT:    mul x11, x15, x14
; CHECK-NEXT:    fmov d0, x9
; CHECK-NEXT:    umulh x14, x15, x14
; CHECK-NEXT:    extr x10, x10, x16, #32
; CHECK-NEXT:    mov v0.d[1], x8
; CHECK-NEXT:    fmov d1, x10
; CHECK-NEXT:    extr x11, x14, x11, #32
; CHECK-NEXT:    mov v1.d[1], x11
; CHECK-NEXT:    ret
  %tmp = call <4 x i64> @llvm.umul.fix.v4i64(<4 x i64> %x, <4 x i64> %y, i32 32)
  ret <4 x i64> %tmp
}

define <4 x i16> @widemul(<4 x i16> %x, <4 x i16> %y) nounwind {
; CHECK-LABEL: widemul:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umull v0.4s, v0.4h, v1.4h
; CHECK-NEXT:    shrn v1.4h, v0.4s, #16
; CHECK-NEXT:    xtn v2.4h, v0.4s
; CHECK-NEXT:    shl v0.4h, v1.4h, #12
; CHECK-NEXT:    usra v0.4h, v2.4h, #4
; CHECK-NEXT:    ret
  %tmp = call <4 x i16> @llvm.umul.fix.v4i16(<4 x i16> %x, <4 x i16> %y, i32 4)
  ret <4 x i16> %tmp
}
