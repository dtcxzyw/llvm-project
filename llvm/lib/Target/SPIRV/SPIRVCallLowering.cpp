//===--- SPIRVCallLowering.cpp - Call lowering ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the lowering of LLVM calls to machine code calls for
// GlobalISel.
//
//===----------------------------------------------------------------------===//

#include "SPIRVCallLowering.h"
#include "MCTargetDesc/SPIRVBaseInfo.h"
#include "SPIRV.h"
#include "SPIRVBuiltins.h"
#include "SPIRVGlobalRegistry.h"
#include "SPIRVISelLowering.h"
#include "SPIRVMetadata.h"
#include "SPIRVRegisterInfo.h"
#include "SPIRVSubtarget.h"
#include "SPIRVUtils.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsSPIRV.h"
#include "llvm/Support/ModRef.h"

using namespace llvm;

SPIRVCallLowering::SPIRVCallLowering(const SPIRVTargetLowering &TLI,
                                     SPIRVGlobalRegistry *GR)
    : CallLowering(&TLI), GR(GR) {}

bool SPIRVCallLowering::lowerReturn(MachineIRBuilder &MIRBuilder,
                                    const Value *Val, ArrayRef<Register> VRegs,
                                    FunctionLoweringInfo &FLI,
                                    Register SwiftErrorVReg) const {
  // Ignore if called from the internal service function
  if (MIRBuilder.getMF()
          .getFunction()
          .getFnAttribute(SPIRV_BACKEND_SERVICE_FUN_NAME)
          .isValid())
    return true;

  // Maybe run postponed production of types for function pointers
  if (IndirectCalls.size() > 0) {
    produceIndirectPtrTypes(MIRBuilder);
    IndirectCalls.clear();
  }

  // Currently all return types should use a single register.
  // TODO: handle the case of multiple registers.
  if (VRegs.size() > 1)
    return false;
  if (Val) {
    const auto &STI = MIRBuilder.getMF().getSubtarget();
    return MIRBuilder.buildInstr(SPIRV::OpReturnValue)
        .addUse(VRegs[0])
        .constrainAllUses(MIRBuilder.getTII(), *STI.getRegisterInfo(),
                          *STI.getRegBankInfo());
  }
  MIRBuilder.buildInstr(SPIRV::OpReturn);
  return true;
}

// Based on the LLVM function attributes, get a SPIR-V FunctionControl.
static uint32_t getFunctionControl(const Function &F,
                                   const SPIRVSubtarget *ST) {
  MemoryEffects MemEffects = F.getMemoryEffects();

  uint32_t FuncControl = static_cast<uint32_t>(SPIRV::FunctionControl::None);

  if (F.hasFnAttribute(Attribute::AttrKind::NoInline))
    FuncControl |= static_cast<uint32_t>(SPIRV::FunctionControl::DontInline);
  else if (F.hasFnAttribute(Attribute::AttrKind::AlwaysInline))
    FuncControl |= static_cast<uint32_t>(SPIRV::FunctionControl::Inline);

  if (MemEffects.doesNotAccessMemory())
    FuncControl |= static_cast<uint32_t>(SPIRV::FunctionControl::Pure);
  else if (MemEffects.onlyReadsMemory())
    FuncControl |= static_cast<uint32_t>(SPIRV::FunctionControl::Const);

  if (ST->canUseExtension(SPIRV::Extension::SPV_INTEL_optnone) ||
      ST->canUseExtension(SPIRV::Extension::SPV_EXT_optnone))
    if (F.hasFnAttribute(Attribute::OptimizeNone))
      FuncControl |= static_cast<uint32_t>(SPIRV::FunctionControl::OptNoneEXT);

  return FuncControl;
}

static ConstantInt *getConstInt(MDNode *MD, unsigned NumOp) {
  if (MD->getNumOperands() > NumOp) {
    auto *CMeta = dyn_cast<ConstantAsMetadata>(MD->getOperand(NumOp));
    if (CMeta)
      return dyn_cast<ConstantInt>(CMeta->getValue());
  }
  return nullptr;
}

// If the function has pointer arguments, we are forced to re-create this
// function type from the very beginning, changing PointerType by
// TypedPointerType for each pointer argument. Otherwise, the same `Type*`
// potentially corresponds to different SPIR-V function type, effectively
// invalidating logic behind global registry and duplicates tracker.
static FunctionType *
fixFunctionTypeIfPtrArgs(SPIRVGlobalRegistry *GR, const Function &F,
                         FunctionType *FTy, const SPIRVType *SRetTy,
                         const SmallVector<SPIRVType *, 4> &SArgTys) {
  bool hasArgPtrs = false;
  for (auto &Arg : F.args()) {
    // check if it's an instance of a non-typed PointerType
    if (Arg.getType()->isPointerTy()) {
      hasArgPtrs = true;
      break;
    }
  }
  if (!hasArgPtrs) {
    Type *RetTy = FTy->getReturnType();
    // check if it's an instance of a non-typed PointerType
    if (!RetTy->isPointerTy())
      return FTy;
  }

  // re-create function type, using TypedPointerType instead of PointerType to
  // properly trace argument types
  const Type *RetTy = GR->getTypeForSPIRVType(SRetTy);
  SmallVector<Type *, 4> ArgTys;
  for (auto SArgTy : SArgTys)
    ArgTys.push_back(const_cast<Type *>(GR->getTypeForSPIRVType(SArgTy)));
  return FunctionType::get(const_cast<Type *>(RetTy), ArgTys, false);
}

// This code restores function args/retvalue types for composite cases
// because the final types should still be aggregate whereas they're i32
// during the translation to cope with aggregate flattening etc.
static FunctionType *getOriginalFunctionType(const Function &F) {
  auto *NamedMD = F.getParent()->getNamedMetadata("spv.cloned_funcs");
  if (NamedMD == nullptr)
    return F.getFunctionType();

  Type *RetTy = F.getFunctionType()->getReturnType();
  SmallVector<Type *, 4> ArgTypes;
  for (auto &Arg : F.args())
    ArgTypes.push_back(Arg.getType());

  auto ThisFuncMDIt =
      std::find_if(NamedMD->op_begin(), NamedMD->op_end(), [&F](MDNode *N) {
        return isa<MDString>(N->getOperand(0)) &&
               cast<MDString>(N->getOperand(0))->getString() == F.getName();
      });
  // TODO: probably one function can have numerous type mutations,
  // so we should support this.
  if (ThisFuncMDIt != NamedMD->op_end()) {
    auto *ThisFuncMD = *ThisFuncMDIt;
    MDNode *MD = dyn_cast<MDNode>(ThisFuncMD->getOperand(1));
    assert(MD && "MDNode operand is expected");
    ConstantInt *Const = getConstInt(MD, 0);
    if (Const) {
      auto *CMeta = dyn_cast<ConstantAsMetadata>(MD->getOperand(1));
      assert(CMeta && "ConstantAsMetadata operand is expected");
      assert(Const->getSExtValue() >= -1);
      // Currently -1 indicates return value, greater values mean
      // argument numbers.
      if (Const->getSExtValue() == -1)
        RetTy = CMeta->getType();
      else
        ArgTypes[Const->getSExtValue()] = CMeta->getType();
    }
  }

  return FunctionType::get(RetTy, ArgTypes, F.isVarArg());
}

static SPIRV::AccessQualifier::AccessQualifier
getArgAccessQual(const Function &F, unsigned ArgIdx) {
  if (F.getCallingConv() != CallingConv::SPIR_KERNEL)
    return SPIRV::AccessQualifier::ReadWrite;

  MDString *ArgAttribute = getOCLKernelArgAccessQual(F, ArgIdx);
  if (!ArgAttribute)
    return SPIRV::AccessQualifier::ReadWrite;

  if (ArgAttribute->getString() == "read_only")
    return SPIRV::AccessQualifier::ReadOnly;
  if (ArgAttribute->getString() == "write_only")
    return SPIRV::AccessQualifier::WriteOnly;
  return SPIRV::AccessQualifier::ReadWrite;
}

static std::vector<SPIRV::Decoration::Decoration>
getKernelArgTypeQual(const Function &F, unsigned ArgIdx) {
  MDString *ArgAttribute = getOCLKernelArgTypeQual(F, ArgIdx);
  if (ArgAttribute && ArgAttribute->getString() == "volatile")
    return {SPIRV::Decoration::Volatile};
  return {};
}

static SPIRVType *getArgSPIRVType(const Function &F, unsigned ArgIdx,
                                  SPIRVGlobalRegistry *GR,
                                  MachineIRBuilder &MIRBuilder,
                                  const SPIRVSubtarget &ST) {
  // Read argument's access qualifier from metadata or default.
  SPIRV::AccessQualifier::AccessQualifier ArgAccessQual =
      getArgAccessQual(F, ArgIdx);

  Type *OriginalArgType = getOriginalFunctionType(F)->getParamType(ArgIdx);

  // If OriginalArgType is non-pointer, use the OriginalArgType (the type cannot
  // be legally reassigned later).
  if (!isPointerTy(OriginalArgType))
    return GR->getOrCreateSPIRVType(OriginalArgType, MIRBuilder, ArgAccessQual,
                                    true);

  Argument *Arg = F.getArg(ArgIdx);
  Type *ArgType = Arg->getType();
  if (isTypedPointerTy(ArgType)) {
    return GR->getOrCreateSPIRVPointerType(
        cast<TypedPointerType>(ArgType)->getElementType(), MIRBuilder,
        addressSpaceToStorageClass(getPointerAddressSpace(ArgType), ST));
  }

  // In case OriginalArgType is of untyped pointer type, there are three
  // possibilities:
  // 1) This is a pointer of an LLVM IR element type, passed byval/byref.
  // 2) This is an OpenCL/SPIR-V builtin type if there is spv_assign_type
  //    intrinsic assigning a TargetExtType.
  // 3) This is a pointer, try to retrieve pointer element type from a
  // spv_assign_ptr_type intrinsic or otherwise use default pointer element
  // type.
  if (hasPointeeTypeAttr(Arg)) {
    return GR->getOrCreateSPIRVPointerType(
        getPointeeTypeByAttr(Arg), MIRBuilder,
        addressSpaceToStorageClass(getPointerAddressSpace(ArgType), ST));
  }

  for (auto User : Arg->users()) {
    auto *II = dyn_cast<IntrinsicInst>(User);
    // Check if this is spv_assign_type assigning OpenCL/SPIR-V builtin type.
    if (II && II->getIntrinsicID() == Intrinsic::spv_assign_type) {
      MetadataAsValue *VMD = cast<MetadataAsValue>(II->getOperand(1));
      Type *BuiltinType =
          cast<ConstantAsMetadata>(VMD->getMetadata())->getType();
      assert(BuiltinType->isTargetExtTy() && "Expected TargetExtType");
      return GR->getOrCreateSPIRVType(BuiltinType, MIRBuilder, ArgAccessQual,
                                      true);
    }

    // Check if this is spv_assign_ptr_type assigning pointer element type.
    if (!II || II->getIntrinsicID() != Intrinsic::spv_assign_ptr_type)
      continue;

    MetadataAsValue *VMD = cast<MetadataAsValue>(II->getOperand(1));
    Type *ElementTy =
        toTypedPointer(cast<ConstantAsMetadata>(VMD->getMetadata())->getType());
    return GR->getOrCreateSPIRVPointerType(
        ElementTy, MIRBuilder,
        addressSpaceToStorageClass(
            cast<ConstantInt>(II->getOperand(2))->getZExtValue(), ST));
  }

  // Replace PointerType with TypedPointerType to be able to map SPIR-V types to
  // LLVM types in a consistent manner
  return GR->getOrCreateSPIRVType(toTypedPointer(OriginalArgType), MIRBuilder,
                                  ArgAccessQual, true);
}

static SPIRV::ExecutionModel::ExecutionModel
getExecutionModel(const SPIRVSubtarget &STI, const Function &F) {
  if (STI.isKernel())
    return SPIRV::ExecutionModel::Kernel;

  if (STI.isShader()) {
    auto attribute = F.getFnAttribute("hlsl.shader");
    if (!attribute.isValid()) {
      report_fatal_error(
          "This entry point lacks mandatory hlsl.shader attribute.");
    }

    const auto value = attribute.getValueAsString();
    if (value == "compute")
      return SPIRV::ExecutionModel::GLCompute;
    if (value == "vertex")
      return SPIRV::ExecutionModel::Vertex;
    if (value == "pixel")
      return SPIRV::ExecutionModel::Fragment;

    report_fatal_error(
        "This HLSL entry point is not supported by this backend.");
  }

  assert(STI.getEnv() == SPIRVSubtarget::Unknown);
  // "hlsl.shader" attribute is mandatory for Vulkan, so we can set Env to
  // Shader whenever we find it, and to Kernel otherwise.

  // We will now change the Env based on the attribute, so we need to strip
  // `const` out of the ref to STI.
  SPIRVSubtarget *NonConstSTI = const_cast<SPIRVSubtarget *>(&STI);
  auto attribute = F.getFnAttribute("hlsl.shader");
  if (!attribute.isValid()) {
    NonConstSTI->setEnv(SPIRVSubtarget::Kernel);
    return SPIRV::ExecutionModel::Kernel;
  }
  NonConstSTI->setEnv(SPIRVSubtarget::Shader);

  const auto value = attribute.getValueAsString();
  if (value == "compute")
    return SPIRV::ExecutionModel::GLCompute;
  if (value == "vertex")
    return SPIRV::ExecutionModel::Vertex;
  if (value == "pixel")
    return SPIRV::ExecutionModel::Fragment;

  report_fatal_error("This HLSL entry point is not supported by this backend.");
}

bool SPIRVCallLowering::lowerFormalArguments(MachineIRBuilder &MIRBuilder,
                                             const Function &F,
                                             ArrayRef<ArrayRef<Register>> VRegs,
                                             FunctionLoweringInfo &FLI) const {
  // Discard the internal service function
  if (F.getFnAttribute(SPIRV_BACKEND_SERVICE_FUN_NAME).isValid())
    return true;

  assert(GR && "Must initialize the SPIRV type registry before lowering args.");
  GR->setCurrentFunc(MIRBuilder.getMF());

  // Get access to information about available extensions
  const SPIRVSubtarget *ST =
      static_cast<const SPIRVSubtarget *>(&MIRBuilder.getMF().getSubtarget());

  // Assign types and names to all args, and store their types for later.
  SmallVector<SPIRVType *, 4> ArgTypeVRegs;
  if (VRegs.size() > 0) {
    unsigned i = 0;
    for (const auto &Arg : F.args()) {
      // Currently formal args should use single registers.
      // TODO: handle the case of multiple registers.
      if (VRegs[i].size() > 1)
        return false;
      auto *SpirvTy = getArgSPIRVType(F, i, GR, MIRBuilder, *ST);
      GR->assignSPIRVTypeToVReg(SpirvTy, VRegs[i][0], MIRBuilder.getMF());
      ArgTypeVRegs.push_back(SpirvTy);

      if (Arg.hasName())
        buildOpName(VRegs[i][0], Arg.getName(), MIRBuilder);
      if (isPointerTyOrWrapper(Arg.getType())) {
        auto DerefBytes = static_cast<unsigned>(Arg.getDereferenceableBytes());
        if (DerefBytes != 0)
          buildOpDecorate(VRegs[i][0], MIRBuilder,
                          SPIRV::Decoration::MaxByteOffset, {DerefBytes});
      }
      if (Arg.hasAttribute(Attribute::Alignment) && !ST->isShader()) {
        auto Alignment = static_cast<unsigned>(
            Arg.getAttribute(Attribute::Alignment).getValueAsInt());
        buildOpDecorate(VRegs[i][0], MIRBuilder, SPIRV::Decoration::Alignment,
                        {Alignment});
      }
      if (Arg.hasAttribute(Attribute::ReadOnly)) {
        auto Attr =
            static_cast<unsigned>(SPIRV::FunctionParameterAttribute::NoWrite);
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr, {Attr});
      }
      if (Arg.hasAttribute(Attribute::ZExt)) {
        auto Attr =
            static_cast<unsigned>(SPIRV::FunctionParameterAttribute::Zext);
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr, {Attr});
      }
      if (Arg.hasAttribute(Attribute::NoAlias)) {
        auto Attr =
            static_cast<unsigned>(SPIRV::FunctionParameterAttribute::NoAlias);
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr, {Attr});
      }
      if (Arg.hasAttribute(Attribute::ByVal)) {
        auto Attr =
            static_cast<unsigned>(SPIRV::FunctionParameterAttribute::ByVal);
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr, {Attr});
      }
      if (Arg.hasAttribute(Attribute::StructRet)) {
        auto Attr =
            static_cast<unsigned>(SPIRV::FunctionParameterAttribute::Sret);
        buildOpDecorate(VRegs[i][0], MIRBuilder,
                        SPIRV::Decoration::FuncParamAttr, {Attr});
      }

      if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
        std::vector<SPIRV::Decoration::Decoration> ArgTypeQualDecs =
            getKernelArgTypeQual(F, i);
        for (SPIRV::Decoration::Decoration Decoration : ArgTypeQualDecs)
          buildOpDecorate(VRegs[i][0], MIRBuilder, Decoration, {});
      }

      MDNode *Node = F.getMetadata("spirv.ParameterDecorations");
      if (Node && i < Node->getNumOperands() &&
          isa<MDNode>(Node->getOperand(i))) {
        MDNode *MD = cast<MDNode>(Node->getOperand(i));
        for (const MDOperand &MDOp : MD->operands()) {
          MDNode *MD2 = dyn_cast<MDNode>(MDOp);
          assert(MD2 && "Metadata operand is expected");
          ConstantInt *Const = getConstInt(MD2, 0);
          assert(Const && "MDOperand should be ConstantInt");
          auto Dec =
              static_cast<SPIRV::Decoration::Decoration>(Const->getZExtValue());
          std::vector<uint32_t> DecVec;
          for (unsigned j = 1; j < MD2->getNumOperands(); j++) {
            ConstantInt *Const = getConstInt(MD2, j);
            assert(Const && "MDOperand should be ConstantInt");
            DecVec.push_back(static_cast<uint32_t>(Const->getZExtValue()));
          }
          buildOpDecorate(VRegs[i][0], MIRBuilder, Dec, DecVec);
        }
      }
      ++i;
    }
  }

  auto MRI = MIRBuilder.getMRI();
  Register FuncVReg = MRI->createGenericVirtualRegister(LLT::scalar(64));
  MRI->setRegClass(FuncVReg, &SPIRV::iIDRegClass);
  FunctionType *FTy = getOriginalFunctionType(F);
  Type *FRetTy = FTy->getReturnType();
  if (isUntypedPointerTy(FRetTy)) {
    if (Type *FRetElemTy = GR->findDeducedElementType(&F)) {
      TypedPointerType *DerivedTy = TypedPointerType::get(
          toTypedPointer(FRetElemTy), getPointerAddressSpace(FRetTy));
      GR->addReturnType(&F, DerivedTy);
      FRetTy = DerivedTy;
    }
  }
  SPIRVType *RetTy = GR->getOrCreateSPIRVType(
      FRetTy, MIRBuilder, SPIRV::AccessQualifier::ReadWrite, true);
  FTy = fixFunctionTypeIfPtrArgs(GR, F, FTy, RetTy, ArgTypeVRegs);
  SPIRVType *FuncTy = GR->getOrCreateOpTypeFunctionWithArgs(
      FTy, RetTy, ArgTypeVRegs, MIRBuilder);
  uint32_t FuncControl = getFunctionControl(F, ST);

  // Add OpFunction instruction
  MachineInstrBuilder MB = MIRBuilder.buildInstr(SPIRV::OpFunction)
                               .addDef(FuncVReg)
                               .addUse(GR->getSPIRVTypeID(RetTy))
                               .addImm(FuncControl)
                               .addUse(GR->getSPIRVTypeID(FuncTy));
  GR->recordFunctionDefinition(&F, &MB.getInstr()->getOperand(0));
  GR->addGlobalObject(&F, &MIRBuilder.getMF(), FuncVReg);
  if (F.isDeclaration())
    GR->add(&F, MB);

  // Add OpFunctionParameter instructions
  int i = 0;
  for (const auto &Arg : F.args()) {
    assert(VRegs[i].size() == 1 && "Formal arg has multiple vregs");
    Register ArgReg = VRegs[i][0];
    MRI->setRegClass(ArgReg, GR->getRegClass(ArgTypeVRegs[i]));
    MRI->setType(ArgReg, GR->getRegType(ArgTypeVRegs[i]));
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpFunctionParameter)
                   .addDef(ArgReg)
                   .addUse(GR->getSPIRVTypeID(ArgTypeVRegs[i]));
    if (F.isDeclaration())
      GR->add(&Arg, MIB);
    GR->addGlobalObject(&Arg, &MIRBuilder.getMF(), ArgReg);
    i++;
  }
  // Name the function.
  if (F.hasName())
    buildOpName(FuncVReg, F.getName(), MIRBuilder);

  // Handle entry points and function linkage.
  if (isEntryPoint(F)) {
    // EntryPoints can help us to determine the environment we're working on.
    // Therefore, we need a non-const pointer to SPIRVSubtarget to update the
    // environment if we need to.
    const SPIRVSubtarget *ST =
        static_cast<const SPIRVSubtarget *>(&MIRBuilder.getMF().getSubtarget());
    auto MIB = MIRBuilder.buildInstr(SPIRV::OpEntryPoint)
                   .addImm(static_cast<uint32_t>(getExecutionModel(*ST, F)))
                   .addUse(FuncVReg);
    addStringImm(F.getName(), MIB);
  } else if (F.getLinkage() != GlobalValue::InternalLinkage &&
             F.getLinkage() != GlobalValue::PrivateLinkage &&
             F.getVisibility() != GlobalValue::HiddenVisibility) {
    SPIRV::LinkageType::LinkageType LnkTy =
        F.isDeclaration()
            ? SPIRV::LinkageType::Import
            : (F.getLinkage() == GlobalValue::LinkOnceODRLinkage &&
                       ST->canUseExtension(
                           SPIRV::Extension::SPV_KHR_linkonce_odr)
                   ? SPIRV::LinkageType::LinkOnceODR
                   : SPIRV::LinkageType::Export);
    buildOpDecorate(FuncVReg, MIRBuilder, SPIRV::Decoration::LinkageAttributes,
                    {static_cast<uint32_t>(LnkTy)}, F.getName());
  }

  // Handle function pointers decoration
  bool hasFunctionPointers =
      ST->canUseExtension(SPIRV::Extension::SPV_INTEL_function_pointers);
  if (hasFunctionPointers) {
    if (F.hasFnAttribute("referenced-indirectly")) {
      assert((F.getCallingConv() != CallingConv::SPIR_KERNEL) &&
             "Unexpected 'referenced-indirectly' attribute of the kernel "
             "function");
      buildOpDecorate(FuncVReg, MIRBuilder,
                      SPIRV::Decoration::ReferencedIndirectlyINTEL, {});
    }
  }

  return true;
}

// Used to postpone producing of indirect function pointer types after all
// indirect calls info is collected
// TODO:
// - add a topological sort of IndirectCalls to ensure the best types knowledge
// - we may need to fix function formal parameter types if they are opaque
//   pointers used as function pointers in these indirect calls
void SPIRVCallLowering::produceIndirectPtrTypes(
    MachineIRBuilder &MIRBuilder) const {
  // Create indirect call data types if any
  MachineFunction &MF = MIRBuilder.getMF();
  for (auto const &IC : IndirectCalls) {
    SPIRVType *SpirvRetTy = GR->getOrCreateSPIRVType(
        IC.RetTy, MIRBuilder, SPIRV::AccessQualifier::ReadWrite, true);
    SmallVector<SPIRVType *, 4> SpirvArgTypes;
    for (size_t i = 0; i < IC.ArgTys.size(); ++i) {
      SPIRVType *SPIRVTy = GR->getOrCreateSPIRVType(
          IC.ArgTys[i], MIRBuilder, SPIRV::AccessQualifier::ReadWrite, true);
      SpirvArgTypes.push_back(SPIRVTy);
      if (!GR->getSPIRVTypeForVReg(IC.ArgRegs[i]))
        GR->assignSPIRVTypeToVReg(SPIRVTy, IC.ArgRegs[i], MF);
    }
    // SPIR-V function type:
    FunctionType *FTy =
        FunctionType::get(const_cast<Type *>(IC.RetTy), IC.ArgTys, false);
    SPIRVType *SpirvFuncTy = GR->getOrCreateOpTypeFunctionWithArgs(
        FTy, SpirvRetTy, SpirvArgTypes, MIRBuilder);
    // SPIR-V pointer to function type:
    SPIRVType *IndirectFuncPtrTy = GR->getOrCreateSPIRVPointerType(
        SpirvFuncTy, MIRBuilder, SPIRV::StorageClass::Function);
    // Correct the Callee type
    GR->assignSPIRVTypeToVReg(IndirectFuncPtrTy, IC.Callee, MF);
  }
}

bool SPIRVCallLowering::lowerCall(MachineIRBuilder &MIRBuilder,
                                  CallLoweringInfo &Info) const {
  // Currently call returns should have single vregs.
  // TODO: handle the case of multiple registers.
  if (Info.OrigRet.Regs.size() > 1)
    return false;
  MachineFunction &MF = MIRBuilder.getMF();
  GR->setCurrentFunc(MF);
  const Function *CF = nullptr;
  std::string DemangledName;
  const Type *OrigRetTy = Info.OrigRet.Ty;

  // Emit a regular OpFunctionCall. If it's an externally declared function,
  // be sure to emit its type and function declaration here. It will be hoisted
  // globally later.
  if (Info.Callee.isGlobal()) {
    std::string FuncName = Info.Callee.getGlobal()->getName().str();
    DemangledName = getOclOrSpirvBuiltinDemangledName(FuncName);
    CF = dyn_cast_or_null<const Function>(Info.Callee.getGlobal());
    // TODO: support constexpr casts and indirect calls.
    if (CF == nullptr)
      return false;
    if (FunctionType *FTy = getOriginalFunctionType(*CF)) {
      OrigRetTy = FTy->getReturnType();
      if (isUntypedPointerTy(OrigRetTy)) {
        if (auto *DerivedRetTy = GR->findReturnType(CF))
          OrigRetTy = DerivedRetTy;
      }
    }
  }

  MachineRegisterInfo *MRI = MIRBuilder.getMRI();
  Register ResVReg =
      Info.OrigRet.Regs.empty() ? Register(0) : Info.OrigRet.Regs[0];
  const auto *ST = static_cast<const SPIRVSubtarget *>(&MF.getSubtarget());

  bool isFunctionDecl = CF && CF->isDeclaration();
  if (isFunctionDecl && !DemangledName.empty()) {
    if (ResVReg.isValid()) {
      if (!GR->getSPIRVTypeForVReg(ResVReg)) {
        const Type *RetTy = OrigRetTy;
        if (auto *PtrRetTy = dyn_cast<PointerType>(OrigRetTy)) {
          const Value *OrigValue = Info.OrigRet.OrigValue;
          if (!OrigValue)
            OrigValue = Info.CB;
          if (OrigValue)
            if (Type *ElemTy = GR->findDeducedElementType(OrigValue))
              RetTy =
                  TypedPointerType::get(ElemTy, PtrRetTy->getAddressSpace());
        }
        setRegClassType(ResVReg, RetTy, GR, MIRBuilder,
                        SPIRV::AccessQualifier::ReadWrite, true);
      }
    } else {
      ResVReg = createVirtualRegister(OrigRetTy, GR, MIRBuilder,
                                      SPIRV::AccessQualifier::ReadWrite, true);
    }
    SmallVector<Register, 8> ArgVRegs;
    for (auto Arg : Info.OrigArgs) {
      assert(Arg.Regs.size() == 1 && "Call arg has multiple VRegs");
      Register ArgReg = Arg.Regs[0];
      ArgVRegs.push_back(ArgReg);
      SPIRVType *SpvType = GR->getSPIRVTypeForVReg(ArgReg);
      if (!SpvType) {
        Type *ArgTy = nullptr;
        if (auto *PtrArgTy = dyn_cast<PointerType>(Arg.Ty)) {
          // If Arg.Ty is an untyped pointer (i.e., ptr [addrspace(...)]) and we
          // don't have access to original value in LLVM IR or info about
          // deduced pointee type, then we should wait with setting the type for
          // the virtual register until pre-legalizer step when we access
          // @llvm.spv.assign.ptr.type.p...(...)'s info.
          if (Arg.OrigValue)
            if (Type *ElemTy = GR->findDeducedElementType(Arg.OrigValue))
              ArgTy =
                  TypedPointerType::get(ElemTy, PtrArgTy->getAddressSpace());
        } else {
          ArgTy = Arg.Ty;
        }
        if (ArgTy) {
          SpvType = GR->getOrCreateSPIRVType(
              ArgTy, MIRBuilder, SPIRV::AccessQualifier::ReadWrite, true);
          GR->assignSPIRVTypeToVReg(SpvType, ArgReg, MF);
        }
      }
      if (!MRI->getRegClassOrNull(ArgReg)) {
        // Either we have SpvType created, or Arg.Ty is an untyped pointer and
        // we know its virtual register's class and type even if we don't know
        // pointee type.
        MRI->setRegClass(ArgReg, SpvType ? GR->getRegClass(SpvType)
                                         : &SPIRV::pIDRegClass);
        MRI->setType(
            ArgReg,
            SpvType ? GR->getRegType(SpvType)
                    : LLT::pointer(cast<PointerType>(Arg.Ty)->getAddressSpace(),
                                   GR->getPointerSize()));
      }
    }
    if (auto Res =
            SPIRV::lowerBuiltin(DemangledName, ST->getPreferredInstructionSet(),
                                MIRBuilder, ResVReg, OrigRetTy, ArgVRegs, GR))
      return *Res;
  }

  if (isFunctionDecl && !GR->find(CF, &MF).isValid()) {
    // Emit the type info and forward function declaration to the first MBB
    // to ensure VReg definition dependencies are valid across all MBBs.
    MachineIRBuilder FirstBlockBuilder;
    FirstBlockBuilder.setMF(MF);
    FirstBlockBuilder.setMBB(*MF.getBlockNumbered(0));

    SmallVector<ArrayRef<Register>, 8> VRegArgs;
    SmallVector<SmallVector<Register, 1>, 8> ToInsert;
    for (const Argument &Arg : CF->args()) {
      if (MIRBuilder.getDataLayout().getTypeStoreSize(Arg.getType()).isZero())
        continue; // Don't handle zero sized types.
      Register Reg = MRI->createGenericVirtualRegister(LLT::scalar(64));
      MRI->setRegClass(Reg, &SPIRV::iIDRegClass);
      ToInsert.push_back({Reg});
      VRegArgs.push_back(ToInsert.back());
    }
    // TODO: Reuse FunctionLoweringInfo
    FunctionLoweringInfo FuncInfo;
    lowerFormalArguments(FirstBlockBuilder, *CF, VRegArgs, FuncInfo);
  }

  // Ignore the call if it's called from the internal service function
  if (MIRBuilder.getMF()
          .getFunction()
          .getFnAttribute(SPIRV_BACKEND_SERVICE_FUN_NAME)
          .isValid()) {
    // insert a no-op
    MIRBuilder.buildTrap();
    return true;
  }

  unsigned CallOp;
  if (Info.CB->isIndirectCall()) {
    if (!ST->canUseExtension(SPIRV::Extension::SPV_INTEL_function_pointers))
      report_fatal_error("An indirect call is encountered but SPIR-V without "
                         "extensions does not support it",
                         false);
    // Set instruction operation according to SPV_INTEL_function_pointers
    CallOp = SPIRV::OpFunctionPointerCallINTEL;
    // Collect information about the indirect call to support possible
    // specification of opaque ptr types of parent function's parameters
    Register CalleeReg = Info.Callee.getReg();
    if (CalleeReg.isValid()) {
      SPIRVCallLowering::SPIRVIndirectCall IndirectCall;
      IndirectCall.Callee = CalleeReg;
      IndirectCall.RetTy = OrigRetTy;
      for (const auto &Arg : Info.OrigArgs) {
        assert(Arg.Regs.size() == 1 && "Call arg has multiple VRegs");
        IndirectCall.ArgTys.push_back(Arg.Ty);
        IndirectCall.ArgRegs.push_back(Arg.Regs[0]);
      }
      IndirectCalls.push_back(IndirectCall);
    }
  } else {
    // Emit a regular OpFunctionCall
    CallOp = SPIRV::OpFunctionCall;
  }

  // Make sure there's a valid return reg, even for functions returning void.
  if (!ResVReg.isValid())
    ResVReg = MIRBuilder.getMRI()->createVirtualRegister(&SPIRV::iIDRegClass);
  SPIRVType *RetType = GR->assignTypeToVReg(
      OrigRetTy, ResVReg, MIRBuilder, SPIRV::AccessQualifier::ReadWrite, true);

  // Emit the call instruction and its args.
  auto MIB = MIRBuilder.buildInstr(CallOp)
                 .addDef(ResVReg)
                 .addUse(GR->getSPIRVTypeID(RetType))
                 .add(Info.Callee);

  for (const auto &Arg : Info.OrigArgs) {
    // Currently call args should have single vregs.
    if (Arg.Regs.size() > 1)
      return false;
    MIB.addUse(Arg.Regs[0]);
  }

  if (ST->canUseExtension(SPIRV::Extension::SPV_INTEL_memory_access_aliasing)) {
    // Process aliasing metadata.
    const CallBase *CI = Info.CB;
    if (CI && CI->hasMetadata()) {
      if (MDNode *MD = CI->getMetadata(LLVMContext::MD_alias_scope))
        GR->buildMemAliasingOpDecorate(ResVReg, MIRBuilder,
                                       SPIRV::Decoration::AliasScopeINTEL, MD);
      if (MDNode *MD = CI->getMetadata(LLVMContext::MD_noalias))
        GR->buildMemAliasingOpDecorate(ResVReg, MIRBuilder,
                                       SPIRV::Decoration::NoAliasINTEL, MD);
    }
  }

  return MIB.constrainAllUses(MIRBuilder.getTII(), *ST->getRegisterInfo(),
                              *ST->getRegBankInfo());
}
