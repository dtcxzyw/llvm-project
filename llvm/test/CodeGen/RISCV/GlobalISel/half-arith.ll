; llc -mtriple=riscv32 -mattr=+f -target-abi=ilp32 -verify-machineinstrs --global-isel test.ll -o -
; llc -mtriple=riscv32 -mattr=+zfh -target-abi=ilp32 -verify-machineinstrs --global-isel test.ll -o -
; llc -mtriple=riscv32 -mattr=+f,+zfhmin -target-abi=ilp32 -verify-machineinstrs --global-isel test.ll -o -
define half @constant() nounwind {
  ret half 0.0
}

define half @constant2() nounwind {
  ret half 120.0
}

define half @fadd(half %a, half %b) nounwind {
  %c = fadd half %a, %b
  ret half %c
}

define half @fsub(half %a, half %b) nounwind {
  %c = fsub half %a, %b
  ret half %c
}

define half @fneg(half %a) nounwind {
  %c = fneg half %a
  ret half %c
}

define half @fmul(half %a, half %b) nounwind {
  %c = fmul half %a, %b
  ret half %c
}

define half @fdiv(half %a, half %b) nounwind {
  %c = fdiv half %a, %b
  ret half %c
}

define half @frem(half %a, half %b) nounwind {
  %c = frem half %a, %b
  ret half %c
}

define float @ext32(half %a) nounwind {
  %c = fpext half %a to float
  ret float %c
}

define double @ext64(half %a) nounwind {
  %c = fpext half %a to double
  ret double %c
}

define half @trunc32(float %a) nounwind {
  %c = fptrunc float %a to half
  ret half %c
}

define half @trunc64(double %a) nounwind {
  %c = fptrunc double %a to half
  ret half %c
}

define i1 @fcmp_olt(half %a, half %b) nounwind {
  %c = fcmp olt half %a, %b
  ret i1 %c
}

define i1 @fcmp_ole(half %a, half %b) nounwind {
  %c = fcmp ole half %a, %b
  ret i1 %c
}

define i1 @fcmp_ogt(half %a, half %b) nounwind {
  %c = fcmp ogt half %a, %b
  ret i1 %c
}

define i1 @fcmp_ult(half %a, half %b) nounwind {
  %c = fcmp ult half %a, %b
  ret i1 %c
}

define i1 @fcmp_ord(half %a, half %b) nounwind {
  %c = fcmp ord half %a, %b
  ret i1 %c
}

define i1 @fcmp_uno(half %a, half %b) nounwind {
  %c = fcmp uno half %a, %b
  ret i1 %c
}

define i1 @fcmp_oeq(half %a, half %b) nounwind {
  %c = fcmp oeq half %a, %b
  ret i1 %c
}

define i1 @fcmp_une(half %a, half %b) nounwind {
  %c = fcmp une half %a, %b
  ret i1 %c
}

define half @sitofp(i32 %a) nounwind {
  %c = sitofp i32 %a to half
  ret half %c
}

define half @uitofp(i32 %a) nounwind {
  %c = uitofp i32 %a to half
  ret half %c
}

define i32 @fptosi(half %a) nounwind {
  %c = fptosi half %a to i32
  ret i32 %c
}

define i32 @fptoui(half %a) nounwind {
  %c = fptoui half %a to i32
  ret i32 %c
}

define half @fma(half %a, half %b, half %c) nounwind {
  %d = call half @llvm.fma.f16(half %a, half %b, half %c)
  ret half %d
}

define half @fmuladd(half %a, half %b, half %c) nounwind {
  %d = call half @llvm.fmuladd.f16(half %a, half %b, half %c)
  ret half %d
}

define half @fabs(half %a) nounwind {
  %c = call half @llvm.fabs.f16(half %a)
  ret half %c
}

define half @sqrt(half %a) nounwind {
  %c = call half @llvm.sqrt.f16(half %a)
  ret half %c
}

define half @fmaximum(half %a, half %b) nounwind {
  %c = call half @llvm.maximum.f16(half %a, half %b)
  ret half %c
}

define half @fminimum(half %a, half %b) nounwind {
  %c = call half @llvm.minimum.f16(half %a, half %b)
  ret half %c
}

define half @fmaxnum(half %a, half %b) nounwind {
  %c = call half @llvm.maxnum.f16(half %a, half %b)
  ret half %c
}

define half @fminnum(half %a, half %b) nounwind {
  %c = call half @llvm.minnum.f16(half %a, half %b)
  ret half %c
}

define half @copysign(half %a, half %b) nounwind {
  %c = call half @llvm.copysign.f16(half %a, half %b)
  ret half %c
}

define i1 @isfpclass(half %a) nounwind {
  %c = call i1 @llvm.is.fpclass.f16(half %a, i32 512)
  ret i1 %c
}
