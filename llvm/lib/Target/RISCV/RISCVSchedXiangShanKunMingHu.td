//==- RISCVSchedXiangShanKunMingHu.td - XiangShan-KunMingHu Scheduling Definitions --*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------------------------------===//

//===---------------------------------------------------------------------------------------------===//

// XiangShan is a high-performance open-source RISC-V processor initiated by
// the Institute of Computing Technology (ICT) of the Chinese Academy of Sciences.
// Source: https://github.com/OpenXiangShan/XiangShan
// Documentation: https://github.com/OpenXiangShan/XiangShan-doc
// XiangShan-KunMingHu is the third generation of XiangShan processor series.

def XiangShanKunMingHuModel : SchedMachineModel {
  let MicroOpBufferSize = 256;
  let LoopMicroOpBufferSize = 48;  // Instruction queue size
  let IssueWidth = 6;  // 6-way decode and dispatch
  let LoadLatency = 4;
  // TODO: checkpoint-based RAT restore
  let MispredictPenalty = 9; // Based on estimate of pipeline depth.
  let CompleteModel = 0;
  let UnsupportedFeatures = [];
}

let SchedModel = XiangShanKunMingHuModel in {
let BufferSize = 16 in {
  def XS3IntDispatch : ProcResource<4>;
  def XS3FpDispatch : ProcResource<4>;
  def XS3MemDispatch : ProcResource<4>;
}

// (ALU+MUL)*2 + (ALU+DIV)*2
let Super = XS3IntDispatch, BufferSize = 32 in {
  def XS3MIQ : ProcResource<1>;
  def XS3DIQ : ProcResource<1>;
}
def XS3ALUGroup1 : ProcResource<2> {
  let Super = XS3MIQ;
}
def XS3ALUGroup2 : ProcResource<2> {
  let Super = XS3DIQ;
}
def XS3MUL : ProcResource<2> {
  let Super = XS3MIQ;
}
def XS3DIV : ProcResource<2> {
  let Super = XS3DIQ;
}
def XS3ALU : ProcResGroup<[XS3ALUGroup1, XS3ALUGroup2]>;

def XS3MISC : ProcResource<1> {
    let Super = XS3IntDispatch;
    let BufferSize = 16;
}

let Super = XS3FpDispatch, BufferSize = 32 in {
  def XS3FMACRS1 : ProcResource<1>;
  def XS3FMACRS2 : ProcResource<1>;
  def XS3FMISCRS : ProcResource<1>;
}
def XS3FMACGroup1 : ProcResource<2> {
  let Super = XS3FMACRS1;
}
def XS3FMACGroup2 : ProcResource<2> {
  let Super = XS3FMACRS2;
}
def XS3FMISC : ProcResource<2> {
  let Super = XS3FMISCRS;
}
def XS3FMAC : ProcResGroup<[XS3FMACGroup1, XS3FMACGroup2]>;

// 3-LD + 2-STA (share one HyRS)
def XS3LDRS : ProcResource<1> {
  let Super = XS3MemDispatch;
  let BufferSize = 64;
}

def XS3STRS : ProcResource<1> {
  let Super = XS3MemDispatch;
  let BufferSize = 32;
}

// Hybrid (Load/Store)
def XS3HyRS : ProcResource<1> {
  let Super = XS3MemDispatch;
  let BufferSize = 32;
}

def XS3LD1 : ProcResource<2> {
  let Super = XS3LDRS;
}
def XS3LD2 : ProcResource<1> {
  let Super = XS3HyRS;
}

def XS3ST1 : ProcResource<1> {
  let Super = XS3STRS;
}
def XS3ST2 : ProcResource<1> {
  let Super = XS3HyRS;
}

def XS3LD : ProcResGroup<[XS3LD1, XS3LD2]>;
def XS3ST : ProcResGroup<[XS3ST1, XS3ST2]>;

// Branching
def : WriteRes<WriteJmp, [XS3MISC]>;
def : WriteRes<WriteJal, [XS3MISC]>;
def : WriteRes<WriteJalr, [XS3MISC]>;

// Integer arithmetic and logic
let Latency = 1 in {
def : WriteRes<WriteIALU, [XS3ALU]>;
def : WriteRes<WriteIALU32, [XS3ALU]>;
def : WriteRes<WriteShiftImm, [XS3ALU]>;
def : WriteRes<WriteShiftImm32, [XS3ALU]>;
def : WriteRes<WriteShiftReg, [XS3ALU]>;
def : WriteRes<WriteShiftReg32, [XS3ALU]>;
}

// Integer multiplication
let Latency = 3 in {
def : WriteRes<WriteIMul, [XS3MUL]>;
def : WriteRes<WriteIMul32, [XS3MUL]>;
}

// Integer division
// TODO: ReleaseAtCycles
// SRT16
def : WriteRes<WriteIDiv, [XS3DIV]> {
  let Latency = 21; // TODO
}
def : WriteRes<WriteIDiv32,  [XS3DIV]> {
  let Latency = 21; // TODO
}

// Zb*
let Latency = 1 in { // TODO
// Zba
def : WriteRes<WriteSHXADD, [XS3ALU]>;
def : WriteRes<WriteSHXADD32, [XS3ALU]>;

// Zbb
def : WriteRes<WriteRotateImm, [XS3ALU]>;
def : WriteRes<WriteRotateImm32, [XS3ALU]>;
def : WriteRes<WriteRotateReg, [XS3ALU]>;
def : WriteRes<WriteRotateReg32, [XS3ALU]>;
def : WriteRes<WriteCLZ, [XS3ALU]>;
def : WriteRes<WriteCLZ32, [XS3ALU]>;
def : WriteRes<WriteCTZ, [XS3ALU]>;
def : WriteRes<WriteCTZ32, [XS3ALU]>;
def : WriteRes<WriteCPOP, [XS3ALU]>;
def : WriteRes<WriteCPOP32, [XS3ALU]>;
def : WriteRes<WriteORCB, [XS3ALU]>;
def : WriteRes<WriteREV8, [XS3ALU]>;

// Zbc
def : WriteRes<WriteCLMUL, [XS3ALU]>;

// Zbs
def : WriteRes<WriteSingleBit, [XS3ALU]>;
def : WriteRes<WriteSingleBitImm, [XS3ALU]>;
def : WriteRes<WriteBEXT, [XS3ALU]>;
def : WriteRes<WriteBEXTI, [XS3ALU]>;

// Zbkb
def : WriteRes<WriteBREV8, [XS3ALU]>;
def : WriteRes<WritePACK, [XS3ALU]>;
def : WriteRes<WritePACK32, [XS3ALU]>;
def : WriteRes<WriteZIP, [XS3ALU]>;

// Zbx
def : WriteRes<WriteXPERM, [XS3ALU]>;
}

// Memory
def : WriteRes<WriteSTB, [XS3ST]>;
def : WriteRes<WriteSTH, [XS3ST]>;
def : WriteRes<WriteSTW, [XS3ST]>;
def : WriteRes<WriteSTD, [XS3ST]>;
def : WriteRes<WriteFST32, [XS3ST]>;
def : WriteRes<WriteFST64, [XS3ST]>;

let Latency = 4 in {
def : WriteRes<WriteLDB, [XS3LD]>;
def : WriteRes<WriteLDH, [XS3LD]>;
def : WriteRes<WriteLDW, [XS3LD]>;
def : WriteRes<WriteLDD, [XS3LD]>;
def : WriteRes<WriteFLD32, [XS3LD]>;
def : WriteRes<WriteFLD64, [XS3LD]>;
}

// Atomic memory
def : WriteRes<WriteAtomicSTW, [XS3ST]>;
def : WriteRes<WriteAtomicSTD, [XS3ST]>;

let Latency = 4 in {
def : WriteRes<WriteAtomicW, [XS3LD]>;
def : WriteRes<WriteAtomicD, [XS3LD]>;
def : WriteRes<WriteAtomicLDW, [XS3LD]>;
def : WriteRes<WriteAtomicLDD, [XS3LD]>;
}

// XiangShan-KunMingHu uses FuDian.
// Documentation: https://github.com/OpenXiangShan/fudian

let Latency = 3 in {
def : WriteRes<WriteFAdd32, [XS3FMAC]>;
def : WriteRes<WriteFSGNJ32, [XS3FMAC]>;
def : WriteRes<WriteFMinMax32, [XS3FMAC]>;
def : WriteRes<WriteFAdd64, [XS3FMAC]>;
def : WriteRes<WriteFSGNJ64, [XS3FMAC]>;
def : WriteRes<WriteFMinMax64, [XS3FMAC]>;

def : WriteRes<WriteFCvtI32ToF32, [XS3FMAC]>;
def : WriteRes<WriteFCvtI32ToF64, [XS3FMAC]>;
def : WriteRes<WriteFCvtI64ToF32, [XS3FMAC]>;
def : WriteRes<WriteFCvtI64ToF64, [XS3FMAC]>;
def : WriteRes<WriteFCvtF32ToI32, [XS3FMAC]>;
def : WriteRes<WriteFCvtF32ToI64, [XS3FMAC]>;
def : WriteRes<WriteFCvtF64ToI32, [XS3FMAC]>;
def : WriteRes<WriteFCvtF64ToI64, [XS3FMAC]>;
def : WriteRes<WriteFCvtF32ToF64, [XS3FMAC]>;
def : WriteRes<WriteFCvtF64ToF32, [XS3FMAC]>;

def : WriteRes<WriteFClass32, [XS3FMAC]>;
def : WriteRes<WriteFClass64, [XS3FMAC]>;
def : WriteRes<WriteFCmp32, [XS3FMAC]>;
def : WriteRes<WriteFCmp64, [XS3FMAC]>;
def : WriteRes<WriteFMovF32ToI32, [XS3FMAC]>;
def : WriteRes<WriteFMovI32ToF32, [XS3FMAC]>;
def : WriteRes<WriteFMovF64ToI64, [XS3FMAC]>;
def : WriteRes<WriteFMovI64ToF64, [XS3FMAC]>;
}

// FP multiplication
let Latency = 3 in {
def : WriteRes<WriteFMul32, [XS3FMAC]>;
def : WriteRes<WriteFMul64, [XS3FMAC]>;
}

let Latency = 5 in {
def : WriteRes<WriteFMA32, [XS3FMAC]>;
def : WriteRes<WriteFMA64, [XS3FMAC]>;
}

// FP division
def : WriteRes<WriteFDiv32, [XS3FMISC]> {
    let Latency = 11;
}
def : WriteRes<WriteFDiv64, [XS3FMISC]> {
    let Latency = 18;
}

def : WriteRes<WriteFSqrt32, [XS3FMISC]> {
    let Latency = 17;
}
def : WriteRes<WriteFSqrt64, [XS3FMISC]> {
    let Latency = 31;
}

// Vector extension
// Documentation: https://github.com/OpenXiangShan/YunSuan
// TODO: add support for V extension

// Others
def : WriteRes<WriteCSR, [XS3MISC]>;
def : WriteRes<WriteNop, []>;

// Move elimination
def : InstRW<[WriteNop], (instrs COPY)>;

// Bypass and advance
// TODO: back-to-back wake up and issue
def : ReadAdvance<ReadJmp, 0>;
def : ReadAdvance<ReadJalr, 0>;
def : ReadAdvance<ReadCSR, 0>;
def : ReadAdvance<ReadStoreData, 0>;
def : ReadAdvance<ReadMemBase, 0>;
def : ReadAdvance<ReadIALU, 0>;
def : ReadAdvance<ReadIALU32, 0>;
def : ReadAdvance<ReadShiftImm, 0>;
def : ReadAdvance<ReadShiftImm32, 0>;
def : ReadAdvance<ReadShiftReg, 0>;
def : ReadAdvance<ReadShiftReg32, 0>;
def : ReadAdvance<ReadIDiv, 0>;
def : ReadAdvance<ReadIDiv32, 0>;
def : ReadAdvance<ReadIMul, 0>;
def : ReadAdvance<ReadIMul32, 0>;
def : ReadAdvance<ReadAtomicWA, 0>;
def : ReadAdvance<ReadAtomicWD, 0>;
def : ReadAdvance<ReadAtomicDA, 0>;
def : ReadAdvance<ReadAtomicDD, 0>;
def : ReadAdvance<ReadAtomicLDW, 0>;
def : ReadAdvance<ReadAtomicLDD, 0>;
def : ReadAdvance<ReadAtomicSTW, 0>;
def : ReadAdvance<ReadAtomicSTD, 0>;
def : ReadAdvance<ReadFStoreData, 0>;
def : ReadAdvance<ReadFMemBase, 0>;
def : ReadAdvance<ReadFAdd32, 0>;
def : ReadAdvance<ReadFAdd64, 0>;
def : ReadAdvance<ReadFMul32, 0>;
def : ReadAdvance<ReadFMul64, 0>;
def : ReadAdvance<ReadFMA32, 0>;
def : ReadAdvance<ReadFMA32Addend, 2>; // Cascade FMA
def : ReadAdvance<ReadFMA64, 0>;
def : ReadAdvance<ReadFMA64Addend, 2>; // Cascade FMA
def : ReadAdvance<ReadFDiv32, 0>;
def : ReadAdvance<ReadFDiv64, 0>;
def : ReadAdvance<ReadFSqrt32, 0>;
def : ReadAdvance<ReadFSqrt64, 0>;
def : ReadAdvance<ReadFCmp32, 0>;
def : ReadAdvance<ReadFCmp64, 0>;
def : ReadAdvance<ReadFSGNJ32, 0>;
def : ReadAdvance<ReadFSGNJ64, 0>;
def : ReadAdvance<ReadFMinMax32, 0>;
def : ReadAdvance<ReadFMinMax64, 0>;
def : ReadAdvance<ReadFCvtF32ToI32, 0>;
def : ReadAdvance<ReadFCvtF32ToI64, 0>;
def : ReadAdvance<ReadFCvtF64ToI32, 0>;
def : ReadAdvance<ReadFCvtF64ToI64, 0>;
def : ReadAdvance<ReadFCvtI32ToF32, 0>;
def : ReadAdvance<ReadFCvtI32ToF64, 0>;
def : ReadAdvance<ReadFCvtI64ToF32, 0>;
def : ReadAdvance<ReadFCvtI64ToF64, 0>;
def : ReadAdvance<ReadFCvtF32ToF64, 0>;
def : ReadAdvance<ReadFCvtF64ToF32, 0>;
def : ReadAdvance<ReadFMovF32ToI32, 0>;
def : ReadAdvance<ReadFMovI32ToF32, 0>;
def : ReadAdvance<ReadFMovF64ToI64, 0>;
def : ReadAdvance<ReadFMovI64ToF64, 0>;
def : ReadAdvance<ReadFClass32, 0>;
def : ReadAdvance<ReadFClass64, 0>;

// Zb*
// Zba
def : ReadAdvance<ReadSHXADD, 0>;
def : ReadAdvance<ReadSHXADD32, 0>;
// Zbb
def : ReadAdvance<ReadRotateImm, 0>;
def : ReadAdvance<ReadRotateImm32, 0>;
def : ReadAdvance<ReadRotateReg, 0>;
def : ReadAdvance<ReadRotateReg32, 0>;
def : ReadAdvance<ReadCLZ, 0>;
def : ReadAdvance<ReadCLZ32, 0>;
def : ReadAdvance<ReadCTZ, 0>;
def : ReadAdvance<ReadCTZ32, 0>;
def : ReadAdvance<ReadCPOP, 0>;
def : ReadAdvance<ReadCPOP32, 0>;
def : ReadAdvance<ReadORCB, 0>;
def : ReadAdvance<ReadREV8, 0>;
// Zbc
def : ReadAdvance<ReadCLMUL, 0>;
// Zbs
def : ReadAdvance<ReadSingleBit, 0>;
def : ReadAdvance<ReadSingleBitImm, 0>;
// Zbkb
def : ReadAdvance<ReadBREV8, 0>;
def : ReadAdvance<ReadPACK, 0>;
def : ReadAdvance<ReadPACK32, 0>;
def : ReadAdvance<ReadZIP, 0>;
// Zbx
def : ReadAdvance<ReadXPERM, 0>;

//===----------------------------------------------------------------------===//
// Unsupported extensions
defm : UnsupportedSchedV;
defm : UnsupportedSchedZfa;
defm : UnsupportedSchedZfh;
defm : UnsupportedSchedSFB;
}
