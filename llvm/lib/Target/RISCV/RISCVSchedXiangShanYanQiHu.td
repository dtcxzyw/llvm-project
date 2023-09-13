//==- RISCVSchedXiangShanYanQiHu.td - XiangShan-YanQiHu Scheduling Definitions --*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===-----------------------------------------------------------------------------------------===//

//===-----------------------------------------------------------------------------------------===//

// XiangShan is a high-performance open-source RISC-V processor initiated by
// the Institute of Computing Technology (ICT) of the Chinese Academy of Sciences.
// Source: https://github.com/OpenXiangShan/XiangShan
// Documentation: https://github.com/OpenXiangShan/XiangShan-doc
// XiangShan-YanQiHu is the first generation of XiangShan processor series.

def XiangShanYanQiHuModel : SchedMachineModel {
  let MicroOpBufferSize = 192;
  let IssueWidth = 6;  // 6-way decode and dispatch
  let LoadLatency = 3;
  let MispredictPenalty = 11; // Based on estimate of pipeline depth.
  let CompleteModel = 0;
  let UnsupportedFeatures = [];
}

let SchedModel = XiangShanYanQiHuModel in {
let BufferSize = 16 in {
  def XS1IntDispatch : ProcResource<4>;
  def XS1FpDispatch : ProcResource<4>;
  def XS1MemDispatch : ProcResource<4>;
}

let Super = XS1IntDispatch, BufferSize = 16 in {
  def XS1ALU : ProcResource<4>;
  def XS1MDU : ProcResource<2>;
  def XS1MISC : ProcResource<1>;
}

let Super = XS1FpDispatch, BufferSize = 16 in {
  def XS1FMAC : ProcResource<4>;
  def XS1FMISC : ProcResource<2>;
}

let Super = XS1MemDispatch, BufferSize = 16 in {
  def XS1LD : ProcResource<2>;
  def XS1ST : ProcResource<2>;
}

// Branching
def : WriteRes<WriteJmp, [XS1MISC]>;
def : WriteRes<WriteJal, [XS1MISC]>;
def : WriteRes<WriteJalr, [XS1MISC]>;

// Integer arithmetic and logic
let Latency = 1 in {
def : WriteRes<WriteIALU, [XS1ALU]>;
def : WriteRes<WriteIALU32, [XS1ALU]>;
def : WriteRes<WriteShiftImm, [XS1ALU]>;
def : WriteRes<WriteShiftImm32, [XS1ALU]>;
def : WriteRes<WriteShiftReg, [XS1ALU]>;
def : WriteRes<WriteShiftReg32, [XS1ALU]>;
}

// Integer multiplication
let Latency = 3 in {
def : WriteRes<WriteIMul, [XS1MDU]>;
def : WriteRes<WriteIMul32, [XS1MDU]>;
}

// Integer division
// TODO: ReleaseAtCycles
// SRT4
def : WriteRes<WriteIDiv, [XS1MDU]> {
  let Latency = 21; // TODO
}
def : WriteRes<WriteIDiv32,  [XS1MDU]> {
  let Latency = 21; // TODO
}

// Memory
def : WriteRes<WriteSTB, [XS1ST]>;
def : WriteRes<WriteSTH, [XS1ST]>;
def : WriteRes<WriteSTW, [XS1ST]>;
def : WriteRes<WriteSTD, [XS1ST]>;
def : WriteRes<WriteFST32, [XS1ST]>;
def : WriteRes<WriteFST64, [XS1ST]>;

let Latency = 3 in {
def : WriteRes<WriteLDB, [XS1LD]>;
def : WriteRes<WriteLDH, [XS1LD]>;
def : WriteRes<WriteLDW, [XS1LD]>;
def : WriteRes<WriteLDD, [XS1LD]>;
def : WriteRes<WriteFLD32, [XS1LD]>;
def : WriteRes<WriteFLD64, [XS1LD]>;
}

// Atomic memory
// XiangShan-Yanqihu is a single-core processor.
def : WriteRes<WriteAtomicSTW, [XS1ST]>;
def : WriteRes<WriteAtomicSTD, [XS1ST]>;

let Latency = 3 in {
def : WriteRes<WriteAtomicW, [XS1LD]>;
def : WriteRes<WriteAtomicD, [XS1LD]>;
def : WriteRes<WriteAtomicLDW, [XS1LD]>;
def : WriteRes<WriteAtomicLDD, [XS1LD]>;
}

// XiangShan-Yanqihu uses Berkeley HardFloat.
// Single precision.
let Latency = 4 in {
def : WriteRes<WriteFAdd32, [XS1FMAC]>;
def : WriteRes<WriteFSGNJ32, [XS1FMAC]>;
def : WriteRes<WriteFMinMax32, [XS1FMAC]>;
}

// Double precision
let Latency = 6 in {
def : WriteRes<WriteFAdd64, [XS1FMAC]>;
def : WriteRes<WriteFSGNJ64, [XS1FMAC]>;
def : WriteRes<WriteFMinMax64, [XS1FMAC]>;
}

// Conversions
let Latency = 2 in {
def : WriteRes<WriteFCvtI32ToF32, [XS1FMAC]>;
def : WriteRes<WriteFCvtI32ToF64, [XS1FMAC]>;
def : WriteRes<WriteFCvtI64ToF32, [XS1FMAC]>;
def : WriteRes<WriteFCvtI64ToF64, [XS1FMAC]>;
def : WriteRes<WriteFCvtF32ToI32, [XS1FMAC]>;
def : WriteRes<WriteFCvtF32ToI64, [XS1FMAC]>;
def : WriteRes<WriteFCvtF64ToI32, [XS1FMAC]>;
def : WriteRes<WriteFCvtF64ToI64, [XS1FMAC]>;
def : WriteRes<WriteFCvtF32ToF64, [XS1FMAC]>;
def : WriteRes<WriteFCvtF64ToF32, [XS1FMAC]>;

def : WriteRes<WriteFClass32, [XS1FMAC]>;
def : WriteRes<WriteFClass64, [XS1FMAC]>;
def : WriteRes<WriteFCmp32, [XS1FMAC]>;
def : WriteRes<WriteFCmp64, [XS1FMAC]>;
def : WriteRes<WriteFMovF32ToI32, [XS1FMAC]>;
def : WriteRes<WriteFMovI32ToF32, [XS1FMAC]>;
def : WriteRes<WriteFMovF64ToI64, [XS1FMAC]>;
def : WriteRes<WriteFMovI64ToF64, [XS1FMAC]>;
}

// FP multiplication
let Latency = 5 in {
def : WriteRes<WriteFMul32, [XS1FMAC]>;
def : WriteRes<WriteFMA32, [XS1FMAC]>;
}

let Latency = 7 in {
def : WriteRes<WriteFMul64, [XS1FMAC]>;
def : WriteRes<WriteFMA64, [XS1FMAC]>;
}

// FP division
let Latency = 20, ReleaseAtCycles = [20] in {
def : WriteRes<WriteFDiv32, [XS1FMISC]>;
def : WriteRes<WriteFDiv64, [XS1FMISC]>;
}

def : WriteRes<WriteFSqrt32, [XS1FMISC]> { let Latency = 20;
                                                      let ReleaseAtCycles = [20]; }
def : WriteRes<WriteFSqrt64, [XS1FMISC]> { let Latency = 25;
                                                      let ReleaseAtCycles = [25]; }

// Others
def : WriteRes<WriteCSR, [XS1MISC]>;
def : WriteRes<WriteNop, []>;

def : InstRW<[WriteIALU], (instrs COPY)>;

// Bypass and advance
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
def : ReadAdvance<ReadFMA32Addend, 0>;
def : ReadAdvance<ReadFMA64, 0>;
def : ReadAdvance<ReadFMA64Addend, 0>;
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

//===----------------------------------------------------------------------===//
// Unsupported extensions
defm : UnsupportedSchedV;
defm : UnsupportedSchedZba;
defm : UnsupportedSchedZbb;
defm : UnsupportedSchedZbc;
defm : UnsupportedSchedZbs;
defm : UnsupportedSchedZbkb;
defm : UnsupportedSchedZbkx;
defm : UnsupportedSchedZfa;
defm : UnsupportedSchedZfh;
defm : UnsupportedSchedSFB;
}
