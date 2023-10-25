//===- RISCVSchedPredicates.td - RISCV Sched Preds -----*- tablegen -*-----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines scheduling predicate definitions that are used by the
// RISCV subtargets.
//
//===----------------------------------------------------------------------===//

// Identify an instruction that effectively transfers a register to another.
def IsCopyIdiomFn : TIIPredicate<"isCopyIdiom",
                                MCOpcodeSwitchStatement<
                                [
                                // mv rd, rs =>
                                // addi rd, rs, 0
                                MCOpcodeSwitchCase<
                                    [ADDI],
                                    MCReturnStatement<CheckAll<[CheckIsRegOperand<1>, CheckIsImmOperand<2>, CheckZeroOperand<2>]>>>,
                                // fmv.s/d rd, rs =>
                                // fadd.s/d rd, rs, rs
                                MCOpcodeSwitchCase<
                                    [FSGNJ_D, FSGNJ_S, FSGNJ_H, FSGNJ_D_INX, FSGNJ_D_IN32X, FSGNJ_S_INX, FSGNJ_H_INX],
                                    MCReturnStatement<CheckAll<[CheckIsRegOperand<1>, CheckIsRegOperand<2>, CheckSameRegOperand<1, 2>]>>>
                                ],
                                MCReturnStatement<CheckFunctionPredicate<"[](const MCInst& MI) { return MI.getOpcode() == RISCV::C_MV; }", 
                                    "[](const MachineInstr& MI) { return MI.isMoveReg(); }">>>>;
def IsCopyIdiomPred : MCSchedPredicate<IsCopyIdiomFn>;
