//===-- RISCVInstrInfoZicldst.td ---------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the RISC-V instructions from the standard Integer
// Conditional Load Store operations extension (Zicldst).
//
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 1, mayStore = 0 in {
class CondLoad_ri<bits<3> funct3, string opcodestr>
    : RVInstI<funct3, OPC_CUSTOM_0, (outs GPR:$rd), (ins GPRMem:$rs1, simm12:$imm12),
              opcodestr, "$rd, ${imm12}(${rs1})">;
}

let hasSideEffects = 0, mayLoad = 0, mayStore = 1 in {
class CondStore_rri<bits<3> funct3, string opcodestr>
    : RVInstS<funct3, OPC_CUSTOM_1, (outs),
              (ins GPR:$rs2, GPRMem:$rs1, simm12:$imm12),
              opcodestr, "$rs2, ${imm12}(${rs1})">;
}

let Predicates = [HasStdExtZicldst] in {

let IsSignExtendingOpW = 1 in {
def CLB  : CondLoad_ri<0b000, "clb">, Sched<[WriteLDB, ReadMemBase]>;
def CLH  : CondLoad_ri<0b001, "clh">, Sched<[WriteLDH, ReadMemBase]>;
def CLW  : CondLoad_ri<0b010, "clw">, Sched<[WriteLDW, ReadMemBase]>;
def CLBU : CondLoad_ri<0b100, "clbu">, Sched<[WriteLDB, ReadMemBase]>;
def CLHU : CondLoad_ri<0b101, "clhu">, Sched<[WriteLDH, ReadMemBase]>;
}

def CSB : CondStore_rri<0b000, "csb">, Sched<[WriteSTB, ReadStoreData, ReadMemBase]>;
def CSH : CondStore_rri<0b001, "csh">, Sched<[WriteSTH, ReadStoreData, ReadMemBase]>;
def CSW : CondStore_rri<0b010, "csw">, Sched<[WriteSTW, ReadStoreData, ReadMemBase]>;

} // Predicates = [HasStdExtZcildst]

let Predicates = [HasStdExtZicldst, IsRV64] in {
def CLWU   : CondLoad_ri<0b110, "clwu">, Sched<[WriteLDW, ReadMemBase]>;
def CLD    : CondLoad_ri<0b011, "cld">, Sched<[WriteLDD, ReadMemBase]>;
def CSD    : CondStore_rri<0b011, "csd">, Sched<[WriteSTD, ReadStoreData, ReadMemBase]>;
} // Predicates = [HasStdExtZicldst, IsRV64]

def SDTRVCLoad : SDTypeProfile<1, 2, [
  SDTCisVT<0, XLenVT>, SDTCisPtrTy<1>, SDTCisVT<2, XLenVT>
]>;
def SDTRVCStore : SDTypeProfile<0, 3, [
  SDTCisVT<0, XLenVT>, SDTCisPtrTy<1>, SDTCisVT<2, XLenVT>
]>;

def riscv_cload : SDNode<"RISCVISD::CLOAD", SDTRVCLoad, [SDNPHasChain, SDNPMayLoad, SDNPMemOperand]>;
def riscv_cstore : SDNode<"RISCVISD::CSTORE", SDTRVCStore, [SDNPHasChain, SDNPMayStore, SDNPMemOperand]>;

def AddrRegZero : ComplexPattern<iPTR, 2, "SelectAddrRegZero">;

let Predicates = [HasStdExtZicldst] in {

def : Pat<(XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 8)), (CLB GPR:$rs1, simm12:$imm12)>;
def : Pat<(XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 16)), (CLH GPR:$rs1, simm12:$imm12)>;
def : Pat<(XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 32)), (CLW GPR:$rs1, simm12:$imm12)>;
def : Pat<(and (XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 8)), 0xFF), (CLBU GPR:$rs1, simm12:$imm12)>;
def : Pat<(and (XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 16)), 0xFFFF), (CLHU GPR:$rs1, simm12:$imm12)>;
def : Pat<(riscv_cstore (XLenVT GPR:$rs2), (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 8), (CSB GPR:$rs2, GPR:$rs1, simm12:$imm12)>;
def : Pat<(riscv_cstore (XLenVT GPR:$rs2), (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 16), (CSH GPR:$rs2, GPR:$rs1, simm12:$imm12)>;
def : Pat<(riscv_cstore (XLenVT GPR:$rs2), (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 32), (CSW GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

} // Predicates = [HasStdExtZicldst]

let Predicates = [HasStdExtZicldst, IsRV64] in {

def : Pat<(and (XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 32)), 0xFFFFFFFF), (CLWU GPR:$rs1, simm12:$imm12)>;
def : Pat<(XLenVT (riscv_cload (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 64)), (CLD GPR:$rs1, simm12:$imm12)>;
def : Pat<(riscv_cstore (XLenVT GPR:$rs2), (AddrRegZero (XLenVT GPR:$rs1), simm12:$imm12), 64), (CSD GPR:$rs2, GPR:$rs1, simm12:$imm12)>;

} // Predicates = [HasStdExtZicldst, IsRV64]
