//===- OptimizedBufferization.cpp - special cases for bufferization -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// In some special cases we can bufferize hlfir expressions in a more optimal
// way so as to avoid creating temporaries. This pass handles these. It should
// be run before the catch-all bufferization pass.
//
// This requires constant subexpression elimination to have already been run.
//===----------------------------------------------------------------------===//

#include "flang/Optimizer/Analysis/AliasAnalysis.h"
#include "flang/Optimizer/Builder/FIRBuilder.h"
#include "flang/Optimizer/Builder/HLFIRTools.h"
#include "flang/Optimizer/Dialect/FIROps.h"
#include "flang/Optimizer/Dialect/FIRType.h"
#include "flang/Optimizer/HLFIR/HLFIRDialect.h"
#include "flang/Optimizer/HLFIR/HLFIROps.h"
#include "flang/Optimizer/HLFIR/Passes.h"
#include "flang/Optimizer/OpenMP/Passes.h"
#include "flang/Optimizer/Support/Utils.h"
#include "flang/Optimizer/Transforms/Utils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "llvm/ADT/TypeSwitch.h"
#include <iterator>
#include <memory>
#include <mlir/Analysis/AliasAnalysis.h>
#include <optional>

namespace hlfir {
#define GEN_PASS_DEF_OPTIMIZEDBUFFERIZATION
#include "flang/Optimizer/HLFIR/Passes.h.inc"
} // namespace hlfir

#define DEBUG_TYPE "opt-bufferization"

namespace {

/// This transformation should match in place modification of arrays.
/// It should match code of the form
/// %array = some.operation // array has shape %shape
/// %expr = hlfir.elemental %shape : [...] {
/// bb0(%arg0: index)
///   %0 = hlfir.designate %array(%arg0)
///   [...] // no other reads or writes to %array
///   hlfir.yield_element %element
/// }
/// hlfir.assign %expr to %array
/// hlfir.destroy %expr
///
/// Or
///
/// %read_array = some.operation // shape %shape
/// %expr = hlfir.elemental %shape : [...] {
/// bb0(%arg0: index)
///   %0 = hlfir.designate %read_array(%arg0)
///   [...]
///   hlfir.yield_element %element
/// }
/// %write_array = some.operation // with shape %shape
/// [...] // operations which don't effect write_array
/// hlfir.assign %expr to %write_array
/// hlfir.destroy %expr
///
/// In these cases, it is safe to turn the elemental into a do loop and modify
/// elements of %array in place without creating an extra temporary for the
/// elemental. We must check that there are no reads from the array at indexes
/// which might conflict with the assignment or any writes. For now we will keep
/// that strict and say that all reads must be at the elemental index (it is
/// probably safe to read from higher indices if lowering to an ordered loop).
class ElementalAssignBufferization
    : public mlir::OpRewritePattern<hlfir::ElementalOp> {
private:
  struct MatchInfo {
    mlir::Value array;
    hlfir::AssignOp assign;
    hlfir::DestroyOp destroy;
  };
  /// determines if the transformation can be applied to this elemental
  static std::optional<MatchInfo> findMatch(hlfir::ElementalOp elemental);

  /// Returns the array indices for the given hlfir.designate.
  /// It recognizes the computations used to transform the one-based indices
  /// into the array's lb-based indices, and returns the one-based indices
  /// in these cases.
  static llvm::SmallVector<mlir::Value>
  getDesignatorIndices(hlfir::DesignateOp designate);

public:
  using mlir::OpRewritePattern<hlfir::ElementalOp>::OpRewritePattern;

  llvm::LogicalResult
  matchAndRewrite(hlfir::ElementalOp elemental,
                  mlir::PatternRewriter &rewriter) const override;
};

/// recursively collect all effects between start and end (including start, not
/// including end) start must properly dominate end, start and end must be in
/// the same block. If any operations with unknown effects are found,
/// std::nullopt is returned
static std::optional<mlir::SmallVector<mlir::MemoryEffects::EffectInstance>>
getEffectsBetween(mlir::Operation *start, mlir::Operation *end) {
  mlir::SmallVector<mlir::MemoryEffects::EffectInstance> ret;
  if (start == end)
    return ret;
  assert(start->getBlock() && end->getBlock() && "TODO: block arguments");
  assert(start->getBlock() == end->getBlock());
  assert(mlir::DominanceInfo{}.properlyDominates(start, end));

  mlir::Operation *nextOp = start;
  while (nextOp && nextOp != end) {
    std::optional<mlir::SmallVector<mlir::MemoryEffects::EffectInstance>>
        effects = mlir::getEffectsRecursively(nextOp);
    if (!effects)
      return std::nullopt;
    ret.append(*effects);
    nextOp = nextOp->getNextNode();
  }
  return ret;
}

/// If effect is a read or write on val, return whether it aliases.
/// Otherwise return mlir::AliasResult::NoAlias
static mlir::AliasResult
containsReadOrWriteEffectOn(const mlir::MemoryEffects::EffectInstance &effect,
                            mlir::Value val) {
  fir::AliasAnalysis aliasAnalysis;

  if (mlir::isa<mlir::MemoryEffects::Read, mlir::MemoryEffects::Write>(
          effect.getEffect())) {
    mlir::Value accessedVal = effect.getValue();
    if (mlir::isa<fir::DebuggingResource>(effect.getResource()))
      return mlir::AliasResult::NoAlias;
    if (!accessedVal)
      return mlir::AliasResult::MayAlias;
    if (accessedVal == val)
      return mlir::AliasResult::MustAlias;

    // if the accessed value might alias val
    mlir::AliasResult res = aliasAnalysis.alias(val, accessedVal);
    if (!res.isNo())
      return res;

    // FIXME: alias analysis of fir.load
    // follow this common pattern:
    // %ref = hlfir.designate %array(%index)
    // %val = fir.load $ref
    if (auto designate = accessedVal.getDefiningOp<hlfir::DesignateOp>()) {
      if (designate.getMemref() == val)
        return mlir::AliasResult::MustAlias;

      // if the designate is into an array that might alias val
      res = aliasAnalysis.alias(val, designate.getMemref());
      if (!res.isNo())
        return res;
    }
  }
  return mlir::AliasResult::NoAlias;
}

// Helper class for analyzing two array slices represented
// by two hlfir.designate operations.
class ArraySectionAnalyzer {
public:
  // The result of the analyzis is one of the values below.
  enum class SlicesOverlapKind {
    // Slices overlap is unknown.
    Unknown,
    // Slices are definitely identical.
    DefinitelyIdentical,
    // Slices are definitely disjoint.
    DefinitelyDisjoint,
    // Slices may be either disjoint or identical,
    // i.e. there is definitely no partial overlap.
    EitherIdenticalOrDisjoint
  };

  // Analyzes two hlfir.designate results and returns the overlap kind.
  // The callers may use this method when the alias analysis reports
  // an alias of some kind, so that we can run Fortran specific analysis
  // on the array slices to see if they are identical or disjoint.
  // Note that the alias analysis are not able to give such an answer
  // about the references.
  static SlicesOverlapKind analyze(mlir::Value ref1, mlir::Value ref2);

private:
  struct SectionDesc {
    // An array section is described by <lb, ub, stride> tuple.
    // If the designator's subscript is not a triple, then
    // the section descriptor is constructed as <lb, nullptr, nullptr>.
    mlir::Value lb, ub, stride;

    SectionDesc(mlir::Value lb, mlir::Value ub, mlir::Value stride)
        : lb(lb), ub(ub), stride(stride) {
      assert(lb && "lower bound or index must be specified");
      normalize();
    }

    // Normalize the section descriptor:
    //   1. If UB is nullptr, then it is set to LB.
    //   2. If LB==UB, then stride does not matter,
    //      so it is reset to nullptr.
    //   3. If STRIDE==1, then it is reset to nullptr.
    void normalize() {
      if (!ub)
        ub = lb;
      if (lb == ub)
        stride = nullptr;
      if (stride)
        if (auto val = fir::getIntIfConstant(stride))
          if (*val == 1)
            stride = nullptr;
    }

    bool operator==(const SectionDesc &other) const {
      return lb == other.lb && ub == other.ub && stride == other.stride;
    }
  };

  // Given an operand_iterator over the indices operands,
  // read the subscript values and return them as SectionDesc
  // updating the iterator. If isTriplet is true,
  // the subscript is a triplet, and the result is <lb, ub, stride>.
  // Otherwise, the subscript is a scalar index, and the result
  // is <index, nullptr, nullptr>.
  static SectionDesc readSectionDesc(mlir::Operation::operand_iterator &it,
                                     bool isTriplet) {
    if (isTriplet)
      return {*it++, *it++, *it++};
    return {*it++, nullptr, nullptr};
  }

  // Return the ordered lower and upper bounds of the section.
  // If stride is known to be non-negative, then the ordered
  // bounds match the <lb, ub> of the descriptor.
  // If stride is known to be negative, then the ordered
  // bounds are <ub, lb> of the descriptor.
  // If stride is unknown, we cannot deduce any order,
  // so the result is <nullptr, nullptr>
  static std::pair<mlir::Value, mlir::Value>
  getOrderedBounds(const SectionDesc &desc) {
    mlir::Value stride = desc.stride;
    // Null stride means stride=1.
    if (!stride)
      return {desc.lb, desc.ub};
    // Reverse the bounds, if stride is negative.
    if (auto val = fir::getIntIfConstant(stride)) {
      if (*val >= 0)
        return {desc.lb, desc.ub};
      else
        return {desc.ub, desc.lb};
    }

    return {nullptr, nullptr};
  }

  // Given two array sections <lb1, ub1, stride1> and
  // <lb2, ub2, stride2>, return true only if the sections
  // are known to be disjoint.
  //
  // For example, for any positive constant C:
  //   X:Y does not overlap with (Y+C):Z
  //   X:Y does not overlap with Z:(X-C)
  static bool areDisjointSections(const SectionDesc &desc1,
                                  const SectionDesc &desc2) {
    auto [lb1, ub1] = getOrderedBounds(desc1);
    auto [lb2, ub2] = getOrderedBounds(desc2);
    if (!lb1 || !lb2)
      return false;
    // Note that this comparison must be made on the ordered bounds,
    // otherwise 'a(x:y:1) = a(z:x-1:-1) + 1' may be incorrectly treated
    // as not overlapping (x=2, y=10, z=9).
    if (isLess(ub1, lb2) || isLess(ub2, lb1))
      return true;
    return false;
  }

  // Given two array sections <lb1, ub1, stride1> and
  // <lb2, ub2, stride2>, return true only if the sections
  // are known to be identical.
  //
  // For example:
  //   <x, x, stride>
  //   <x, nullptr, nullptr>
  //
  // These sections are identical, from the point of which array
  // elements are being addresses, even though the shape
  // of the array slices might be different.
  static bool areIdenticalSections(const SectionDesc &desc1,
                                   const SectionDesc &desc2) {
    if (desc1 == desc2)
      return true;
    return false;
  }

  // Return true, if v1 is known to be less than v2.
  static bool isLess(mlir::Value v1, mlir::Value v2);
};

ArraySectionAnalyzer::SlicesOverlapKind
ArraySectionAnalyzer::analyze(mlir::Value ref1, mlir::Value ref2) {
  if (ref1 == ref2)
    return SlicesOverlapKind::DefinitelyIdentical;

  auto des1 = ref1.getDefiningOp<hlfir::DesignateOp>();
  auto des2 = ref2.getDefiningOp<hlfir::DesignateOp>();
  // We only support a pair of designators right now.
  if (!des1 || !des2)
    return SlicesOverlapKind::Unknown;

  if (des1.getMemref() != des2.getMemref()) {
    // If the bases are different, then there is unknown overlap.
    LLVM_DEBUG(llvm::dbgs() << "No identical base for:\n"
                            << des1 << "and:\n"
                            << des2 << "\n");
    return SlicesOverlapKind::Unknown;
  }

  // Require all components of the designators to be the same.
  // It might be too strict, e.g. we may probably allow for
  // different type parameters.
  if (des1.getComponent() != des2.getComponent() ||
      des1.getComponentShape() != des2.getComponentShape() ||
      des1.getSubstring() != des2.getSubstring() ||
      des1.getComplexPart() != des2.getComplexPart() ||
      des1.getTypeparams() != des2.getTypeparams()) {
    LLVM_DEBUG(llvm::dbgs() << "Different designator specs for:\n"
                            << des1 << "and:\n"
                            << des2 << "\n");
    return SlicesOverlapKind::Unknown;
  }

  // Analyze the subscripts.
  auto des1It = des1.getIndices().begin();
  auto des2It = des2.getIndices().begin();
  bool identicalTriplets = true;
  bool identicalIndices = true;
  for (auto [isTriplet1, isTriplet2] :
       llvm::zip(des1.getIsTriplet(), des2.getIsTriplet())) {
    SectionDesc desc1 = readSectionDesc(des1It, isTriplet1);
    SectionDesc desc2 = readSectionDesc(des2It, isTriplet2);

    // See if we can prove that any of the sections do not overlap.
    // This is mostly a Polyhedron/nf performance hack that looks for
    // particular relations between the lower and upper bounds
    // of the array sections, e.g. for any positive constant C:
    //   X:Y does not overlap with (Y+C):Z
    //   X:Y does not overlap with Z:(X-C)
    if (areDisjointSections(desc1, desc2))
      return SlicesOverlapKind::DefinitelyDisjoint;

    if (!areIdenticalSections(desc1, desc2)) {
      if (isTriplet1 || isTriplet2) {
        // For example:
        //   hlfir.designate %6#0 (%c2:%c7999:%c1, %c1:%c120:%c1, %0)
        //   hlfir.designate %6#0 (%c2:%c7999:%c1, %c1:%c120:%c1, %1)
        //
        // If all the triplets (section speficiers) are the same, then
        // we do not care if %0 is equal to %1 - the slices are either
        // identical or completely disjoint.
        //
        // Also, treat these as identical sections:
        //   hlfir.designate %6#0 (%c2:%c2:%c1)
        //   hlfir.designate %6#0 (%c2)
        identicalTriplets = false;
        LLVM_DEBUG(llvm::dbgs() << "Triplet mismatch for:\n"
                                << des1 << "and:\n"
                                << des2 << "\n");
      } else {
        identicalIndices = false;
        LLVM_DEBUG(llvm::dbgs() << "Indices mismatch for:\n"
                                << des1 << "and:\n"
                                << des2 << "\n");
      }
    }
  }

  if (identicalTriplets) {
    if (identicalIndices)
      return SlicesOverlapKind::DefinitelyIdentical;
    else
      return SlicesOverlapKind::EitherIdenticalOrDisjoint;
  }

  LLVM_DEBUG(llvm::dbgs() << "Different sections for:\n"
                          << des1 << "and:\n"
                          << des2 << "\n");
  return SlicesOverlapKind::Unknown;
}

bool ArraySectionAnalyzer::isLess(mlir::Value v1, mlir::Value v2) {
  auto removeConvert = [](mlir::Value v) -> mlir::Operation * {
    auto *op = v.getDefiningOp();
    while (auto conv = mlir::dyn_cast_or_null<fir::ConvertOp>(op))
      op = conv.getValue().getDefiningOp();
    return op;
  };

  auto isPositiveConstant = [](mlir::Value v) -> bool {
    if (auto val = fir::getIntIfConstant(v))
      return *val > 0;
    return false;
  };

  auto *op1 = removeConvert(v1);
  auto *op2 = removeConvert(v2);
  if (!op1 || !op2)
    return false;

  // Check if they are both constants.
  if (auto val1 = fir::getIntIfConstant(op1->getResult(0)))
    if (auto val2 = fir::getIntIfConstant(op2->getResult(0)))
      return *val1 < *val2;

  // Handle some variable cases (C > 0):
  //   v2 = v1 + C
  //   v2 = C + v1
  //   v1 = v2 - C
  if (auto addi = mlir::dyn_cast<mlir::arith::AddIOp>(op2))
    if ((addi.getLhs().getDefiningOp() == op1 &&
         isPositiveConstant(addi.getRhs())) ||
        (addi.getRhs().getDefiningOp() == op1 &&
         isPositiveConstant(addi.getLhs())))
      return true;
  if (auto subi = mlir::dyn_cast<mlir::arith::SubIOp>(op1))
    if (subi.getLhs().getDefiningOp() == op2 &&
        isPositiveConstant(subi.getRhs()))
      return true;
  return false;
}

llvm::SmallVector<mlir::Value>
ElementalAssignBufferization::getDesignatorIndices(
    hlfir::DesignateOp designate) {
  mlir::Value memref = designate.getMemref();

  // If the object is a box, then the indices may be adjusted
  // according to the box's lower bound(s). Scan through
  // the computations to try to find the one-based indices.
  if (mlir::isa<fir::BaseBoxType>(memref.getType())) {
    // Look for the following pattern:
    //   %13 = fir.load %12 : !fir.ref<!fir.box<...>
    //   %14:3 = fir.box_dims %13, %c0 : (!fir.box<...>, index) -> ...
    //   %17 = arith.subi %14#0, %c1 : index
    //   %18 = arith.addi %arg2, %17 : index
    //   %19 = hlfir.designate %13 (%18)  : (!fir.box<...>, index) -> ...
    //
    // %arg2 is a one-based index.

    auto isNormalizedLb = [memref](mlir::Value v, unsigned dim) {
      // Return true, if v and dim are such that:
      //   %14:3 = fir.box_dims %13, %dim : (!fir.box<...>, index) -> ...
      //   %17 = arith.subi %14#0, %c1 : index
      //   %19 = hlfir.designate %13 (...)  : (!fir.box<...>, index) -> ...
      if (auto subOp =
              mlir::dyn_cast_or_null<mlir::arith::SubIOp>(v.getDefiningOp())) {
        auto cst = fir::getIntIfConstant(subOp.getRhs());
        if (!cst || *cst != 1)
          return false;
        if (auto dimsOp = mlir::dyn_cast_or_null<fir::BoxDimsOp>(
                subOp.getLhs().getDefiningOp())) {
          if (memref != dimsOp.getVal() ||
              dimsOp.getResult(0) != subOp.getLhs())
            return false;
          auto dimsOpDim = fir::getIntIfConstant(dimsOp.getDim());
          return dimsOpDim && dimsOpDim == dim;
        }
      }
      return false;
    };

    llvm::SmallVector<mlir::Value> newIndices;
    for (auto index : llvm::enumerate(designate.getIndices())) {
      if (auto addOp = mlir::dyn_cast_or_null<mlir::arith::AddIOp>(
              index.value().getDefiningOp())) {
        for (unsigned opNum = 0; opNum < 2; ++opNum)
          if (isNormalizedLb(addOp->getOperand(opNum), index.index())) {
            newIndices.push_back(addOp->getOperand((opNum + 1) % 2));
            break;
          }

        // If new one-based index was not added, exit early.
        if (newIndices.size() <= index.index())
          break;
      }
    }

    // If any of the indices is not adjusted to the array's lb,
    // then return the original designator indices.
    if (newIndices.size() != designate.getIndices().size())
      return designate.getIndices();

    return newIndices;
  }

  return designate.getIndices();
}

std::optional<ElementalAssignBufferization::MatchInfo>
ElementalAssignBufferization::findMatch(hlfir::ElementalOp elemental) {
  mlir::Operation::user_range users = elemental->getUsers();
  // the only uses of the elemental should be the assignment and the destroy
  if (std::distance(users.begin(), users.end()) != 2) {
    LLVM_DEBUG(llvm::dbgs() << "Too many uses of the elemental\n");
    return std::nullopt;
  }

  // If the ElementalOp must produce a temporary (e.g. for
  // finalization purposes), then we cannot inline it.
  if (hlfir::elementalOpMustProduceTemp(elemental)) {
    LLVM_DEBUG(llvm::dbgs() << "ElementalOp must produce a temp\n");
    return std::nullopt;
  }

  MatchInfo match;
  for (mlir::Operation *user : users)
    mlir::TypeSwitch<mlir::Operation *, void>(user)
        .Case([&](hlfir::AssignOp op) { match.assign = op; })
        .Case([&](hlfir::DestroyOp op) { match.destroy = op; });

  if (!match.assign || !match.destroy) {
    LLVM_DEBUG(llvm::dbgs() << "Couldn't find assign or destroy\n");
    return std::nullopt;
  }

  // the array is what the elemental is assigned into
  // TODO: this could be extended to also allow hlfir.expr by first bufferizing
  // the incoming expression
  match.array = match.assign.getLhs();
  mlir::Type arrayType = mlir::dyn_cast<fir::SequenceType>(
      fir::unwrapPassByRefType(match.array.getType()));
  if (!arrayType) {
    LLVM_DEBUG(llvm::dbgs() << "AssignOp's result is not an array\n");
    return std::nullopt;
  }

  // require that the array elements are trivial
  // TODO: this is just to make the pass easier to think about. Not an inherent
  // limitation
  mlir::Type eleTy = hlfir::getFortranElementType(arrayType);
  if (!fir::isa_trivial(eleTy)) {
    LLVM_DEBUG(llvm::dbgs() << "AssignOp's data type is not trivial\n");
    return std::nullopt;
  }

  // The array must have the same shape as the elemental.
  //
  // f2018 10.2.1.2 (3) requires the lhs and rhs of an assignment to be
  // conformable unless the lhs is an allocatable array. In HLFIR we can
  // see this from the presence or absence of the realloc attribute on
  // hlfir.assign. If it is not a realloc assignment, we can trust that
  // the shapes do conform.
  //
  // TODO: the lhs's shape is dynamic, so it is hard to prove that
  // there is no reallocation of the lhs due to the assignment.
  // We can probably try generating multiple versions of the code
  // with checking for the shape match, length parameters match, etc.
  if (match.assign.isAllocatableAssignment()) {
    LLVM_DEBUG(llvm::dbgs() << "AssignOp may involve (re)allocation of LHS\n");
    return std::nullopt;
  }

  // the transformation wants to apply the elemental in a do-loop at the
  // hlfir.assign, check there are no effects which make this unsafe

  // keep track of any values written to in the elemental, as these can't be
  // read from or written to between the elemental and the assignment
  mlir::SmallVector<mlir::Value, 1> notToBeAccessedBeforeAssign;
  // likewise, values read in the elemental cannot be written to between the
  // elemental and the assign
  mlir::SmallVector<mlir::Value, 1> notToBeWrittenBeforeAssign;

  // 1) side effects in the elemental body - it isn't sufficient to just look
  // for ordered elementals because we also cannot support out of order reads
  std::optional<mlir::SmallVector<mlir::MemoryEffects::EffectInstance>>
      effects = getEffectsBetween(&elemental.getBody()->front(),
                                  elemental.getBody()->getTerminator());
  if (!effects) {
    LLVM_DEBUG(llvm::dbgs()
               << "operation with unknown effects inside elemental\n");
    return std::nullopt;
  }
  for (const mlir::MemoryEffects::EffectInstance &effect : *effects) {
    mlir::AliasResult res = containsReadOrWriteEffectOn(effect, match.array);
    if (res.isNo()) {
      if (effect.getValue()) {
        if (mlir::isa<mlir::MemoryEffects::Write>(effect.getEffect()))
          notToBeAccessedBeforeAssign.push_back(effect.getValue());
        else if (mlir::isa<mlir::MemoryEffects::Read>(effect.getEffect()))
          notToBeWrittenBeforeAssign.push_back(effect.getValue());
      }

      // this is safe in the elemental
      continue;
    }

    // don't allow any aliasing writes in the elemental
    if (mlir::isa<mlir::MemoryEffects::Write>(effect.getEffect())) {
      LLVM_DEBUG(llvm::dbgs() << "write inside the elemental body\n");
      return std::nullopt;
    }

    if (effect.getValue() == nullptr) {
      LLVM_DEBUG(llvm::dbgs()
                 << "side-effect with no value, cannot analyze further\n");
      return std::nullopt;
    }

    // allow if and only if the reads are from the elemental indices, in order
    // => each iteration doesn't read values written by other iterations
    // don't allow reads from a different value which may alias: fir alias
    // analysis isn't precise enough to tell us if two aliasing arrays overlap
    // exactly or only partially. If they overlap partially, a designate at the
    // elemental indices could be accessing different elements: e.g. we could
    // designate two slices of the same array at different start indexes. These
    // two MustAlias but index 1 of one array isn't the same element as index 1
    // of the other array.
    if (!res.isPartial()) {
      if (auto designate =
              effect.getValue().getDefiningOp<hlfir::DesignateOp>()) {
        ArraySectionAnalyzer::SlicesOverlapKind overlap =
            ArraySectionAnalyzer::analyze(match.array, designate.getMemref());
        if (overlap ==
            ArraySectionAnalyzer::SlicesOverlapKind::DefinitelyDisjoint)
          continue;

        if (overlap == ArraySectionAnalyzer::SlicesOverlapKind::Unknown) {
          LLVM_DEBUG(llvm::dbgs() << "possible read conflict: " << designate
                                  << " at " << elemental.getLoc() << "\n");
          return std::nullopt;
        }
        auto indices = getDesignatorIndices(designate);
        auto elementalIndices = elemental.getIndices();
        if (indices.size() == elementalIndices.size() &&
            std::equal(indices.begin(), indices.end(), elementalIndices.begin(),
                       elementalIndices.end()))
          continue;

        LLVM_DEBUG(llvm::dbgs() << "possible read conflict: " << designate
                                << " at " << elemental.getLoc() << "\n");
        return std::nullopt;
      }
    }
    LLVM_DEBUG(llvm::dbgs() << "disallowed side-effect: " << effect.getValue()
                            << " for " << elemental.getLoc() << "\n");
    return std::nullopt;
  }

  // 2) look for conflicting effects between the elemental and the assignment
  effects = getEffectsBetween(elemental->getNextNode(), match.assign);
  if (!effects) {
    LLVM_DEBUG(
        llvm::dbgs()
        << "operation with unknown effects between elemental and assign\n");
    return std::nullopt;
  }
  for (const mlir::MemoryEffects::EffectInstance &effect : *effects) {
    // not safe to access anything written in the elemental as this write
    // will be moved to the assignment
    for (mlir::Value val : notToBeAccessedBeforeAssign) {
      mlir::AliasResult res = containsReadOrWriteEffectOn(effect, val);
      if (!res.isNo()) {
        LLVM_DEBUG(llvm::dbgs()
                   << "disallowed side-effect: " << effect.getValue() << " for "
                   << elemental.getLoc() << "\n");
        return std::nullopt;
      }
    }
    // Anything that is read inside the elemental can only be safely read
    // between the elemental and the assignment.
    for (mlir::Value val : notToBeWrittenBeforeAssign) {
      mlir::AliasResult res = containsReadOrWriteEffectOn(effect, val);
      if (!res.isNo() &&
          !mlir::isa<mlir::MemoryEffects::Read>(effect.getEffect())) {
        LLVM_DEBUG(llvm::dbgs()
                   << "disallowed non-read side-effect: " << effect.getValue()
                   << " for " << elemental.getLoc() << "\n");
        return std::nullopt;
      }
    }
  }

  return match;
}

llvm::LogicalResult ElementalAssignBufferization::matchAndRewrite(
    hlfir::ElementalOp elemental, mlir::PatternRewriter &rewriter) const {
  std::optional<MatchInfo> match = findMatch(elemental);
  if (!match)
    return rewriter.notifyMatchFailure(
        elemental, "cannot prove safety of ElementalAssignBufferization");

  mlir::Location loc = elemental->getLoc();
  fir::FirOpBuilder builder(rewriter, elemental.getOperation());
  auto rhsExtents = hlfir::getIndexExtents(loc, builder, elemental.getShape());

  // create the loop at the assignment
  builder.setInsertionPoint(match->assign);
  hlfir::Entity lhs{match->array};
  lhs = hlfir::derefPointersAndAllocatables(loc, builder, lhs);
  mlir::Value lhsShape = hlfir::genShape(loc, builder, lhs);
  llvm::SmallVector<mlir::Value> lhsExtents =
      hlfir::getIndexExtents(loc, builder, lhsShape);
  llvm::SmallVector<mlir::Value> extents =
      fir::factory::deduceOptimalExtents(rhsExtents, lhsExtents);

  // Generate a loop nest looping around the hlfir.elemental shape and clone
  // hlfir.elemental region inside the inner loop
  hlfir::LoopNest loopNest =
      hlfir::genLoopNest(loc, builder, extents, !elemental.isOrdered(),
                         flangomp::shouldUseWorkshareLowering(elemental));
  builder.setInsertionPointToStart(loopNest.body);
  auto yield = hlfir::inlineElementalOp(loc, builder, elemental,
                                        loopNest.oneBasedIndices);
  hlfir::Entity elementValue{yield.getElementValue()};
  rewriter.eraseOp(yield);

  // Assign the element value to the array element for this iteration.
  auto arrayElement =
      hlfir::getElementAt(loc, builder, lhs, loopNest.oneBasedIndices);
  hlfir::AssignOp::create(
      builder, loc, elementValue, arrayElement, /*realloc=*/false,
      /*keep_lhs_length_if_realloc=*/false, match->assign.getTemporaryLhs());

  rewriter.eraseOp(match->assign);
  rewriter.eraseOp(match->destroy);
  rewriter.eraseOp(elemental);
  return mlir::success();
}

/// Expand hlfir.assign of a scalar RHS to array LHS into a loop nest
/// of element-by-element assignments:
///   hlfir.assign %cst to %0 : f32, !fir.ref<!fir.array<6x6xf32>>
/// into:
///   fir.do_loop %arg0 = %c1 to %c6 step %c1 unordered {
///     fir.do_loop %arg1 = %c1 to %c6 step %c1 unordered {
///       %1 = hlfir.designate %0 (%arg1, %arg0)  :
///       (!fir.ref<!fir.array<6x6xf32>>, index, index) -> !fir.ref<f32>
///       hlfir.assign %cst to %1 : f32, !fir.ref<f32>
///     }
///   }
class BroadcastAssignBufferization
    : public mlir::OpRewritePattern<hlfir::AssignOp> {
private:
public:
  using mlir::OpRewritePattern<hlfir::AssignOp>::OpRewritePattern;

  llvm::LogicalResult
  matchAndRewrite(hlfir::AssignOp assign,
                  mlir::PatternRewriter &rewriter) const override;
};

llvm::LogicalResult BroadcastAssignBufferization::matchAndRewrite(
    hlfir::AssignOp assign, mlir::PatternRewriter &rewriter) const {
  // Since RHS is a scalar and LHS is an array, LHS must be allocated
  // in a conforming Fortran program, and LHS cannot be reallocated
  // as a result of the assignment. So we can ignore isAllocatableAssignment
  // and do the transformation always.
  mlir::Value rhs = assign.getRhs();
  if (!fir::isa_trivial(rhs.getType()))
    return rewriter.notifyMatchFailure(
        assign, "AssignOp's RHS is not a trivial scalar");

  hlfir::Entity lhs{assign.getLhs()};
  if (!lhs.isArray())
    return rewriter.notifyMatchFailure(assign,
                                       "AssignOp's LHS is not an array");

  mlir::Type eleTy = lhs.getFortranElementType();
  if (!fir::isa_trivial(eleTy))
    return rewriter.notifyMatchFailure(
        assign, "AssignOp's LHS data type is not trivial");

  mlir::Location loc = assign->getLoc();
  fir::FirOpBuilder builder(rewriter, assign.getOperation());
  builder.setInsertionPoint(assign);
  lhs = hlfir::derefPointersAndAllocatables(loc, builder, lhs);
  mlir::Value shape = hlfir::genShape(loc, builder, lhs);
  llvm::SmallVector<mlir::Value> extents =
      hlfir::getIndexExtents(loc, builder, shape);

  if (lhs.isSimplyContiguous() && extents.size() > 1) {
    // Flatten the array to use a single assign loop, that can be better
    // optimized.
    mlir::Value n = extents[0];
    for (size_t i = 1; i < extents.size(); ++i)
      n = mlir::arith::MulIOp::create(builder, loc, n, extents[i]);
    llvm::SmallVector<mlir::Value> flatExtents = {n};

    mlir::Type flatArrayType;
    mlir::Value flatArray = lhs.getBase();
    if (mlir::isa<fir::BoxType>(lhs.getType())) {
      shape = builder.genShape(loc, flatExtents);
      flatArrayType = fir::BoxType::get(fir::SequenceType::get(eleTy, 1));
      flatArray = fir::ReboxOp::create(builder, loc, flatArrayType, flatArray,
                                       shape, /*slice=*/mlir::Value{});
    } else {
      // Array references must have fixed shape, when used in assignments.
      auto seqTy =
          mlir::cast<fir::SequenceType>(fir::unwrapRefType(lhs.getType()));
      llvm::ArrayRef<int64_t> fixedShape = seqTy.getShape();
      int64_t flatExtent = 1;
      for (int64_t extent : fixedShape)
        flatExtent *= extent;
      flatArrayType =
          fir::ReferenceType::get(fir::SequenceType::get({flatExtent}, eleTy));
      flatArray = builder.createConvert(loc, flatArrayType, flatArray);
    }

    hlfir::LoopNest loopNest =
        hlfir::genLoopNest(loc, builder, flatExtents, /*isUnordered=*/true,
                           flangomp::shouldUseWorkshareLowering(assign));
    builder.setInsertionPointToStart(loopNest.body);

    mlir::Value arrayElement =
        hlfir::DesignateOp::create(builder, loc, fir::ReferenceType::get(eleTy),
                                   flatArray, loopNest.oneBasedIndices);
    hlfir::AssignOp::create(builder, loc, rhs, arrayElement);
  } else {
    hlfir::LoopNest loopNest =
        hlfir::genLoopNest(loc, builder, extents, /*isUnordered=*/true,
                           flangomp::shouldUseWorkshareLowering(assign));
    builder.setInsertionPointToStart(loopNest.body);
    auto arrayElement =
        hlfir::getElementAt(loc, builder, lhs, loopNest.oneBasedIndices);
    hlfir::AssignOp::create(builder, loc, rhs, arrayElement);
  }

  rewriter.eraseOp(assign);
  return mlir::success();
}

class EvaluateIntoMemoryAssignBufferization
    : public mlir::OpRewritePattern<hlfir::EvaluateInMemoryOp> {

public:
  using mlir::OpRewritePattern<hlfir::EvaluateInMemoryOp>::OpRewritePattern;

  llvm::LogicalResult
  matchAndRewrite(hlfir::EvaluateInMemoryOp,
                  mlir::PatternRewriter &rewriter) const override;
};

static llvm::LogicalResult
tryUsingAssignLhsDirectly(hlfir::EvaluateInMemoryOp evalInMem,
                          mlir::PatternRewriter &rewriter) {
  mlir::Location loc = evalInMem.getLoc();
  hlfir::DestroyOp destroy;
  hlfir::AssignOp assign;
  for (auto user : llvm::enumerate(evalInMem->getUsers())) {
    if (user.index() > 2)
      return mlir::failure();
    mlir::TypeSwitch<mlir::Operation *, void>(user.value())
        .Case([&](hlfir::AssignOp op) { assign = op; })
        .Case([&](hlfir::DestroyOp op) { destroy = op; });
  }
  if (!assign || !destroy || destroy.mustFinalizeExpr() ||
      assign.isAllocatableAssignment())
    return mlir::failure();

  hlfir::Entity lhs{assign.getLhs()};
  // EvaluateInMemoryOp memory is contiguous, so in general, it can only be
  // replace by the LHS if the LHS is contiguous.
  if (!lhs.isSimplyContiguous())
    return mlir::failure();
  // Character assignment may involves truncation/padding, so the LHS
  // cannot be used to evaluate RHS in place without proving the LHS and
  // RHS lengths are the same.
  if (lhs.isCharacter())
    return mlir::failure();
  fir::AliasAnalysis aliasAnalysis;
  // The region must not read or write the LHS.
  // Note that getModRef is used instead of mlir::MemoryEffects because
  // EvaluateInMemoryOp is typically expected to hold fir.calls and that
  // Fortran calls cannot be modeled in a useful way with mlir::MemoryEffects:
  // it is hard/impossible to list all the read/written SSA values in a call,
  // but it is often possible to tell that an SSA value cannot be accessed,
  // hence getModRef is needed here and below. Also note that getModRef uses
  // mlir::MemoryEffects for operations that do not have special handling in
  // getModRef.
  if (aliasAnalysis.getModRef(evalInMem.getBody(), lhs).isModOrRef())
    return mlir::failure();
  // Any variables affected between the hlfir.evalInMem and assignment must not
  // be read or written inside the region since it will be moved at the
  // assignment insertion point.
  auto effects = getEffectsBetween(evalInMem->getNextNode(), assign);
  if (!effects) {
    LLVM_DEBUG(
        llvm::dbgs()
        << "operation with unknown effects between eval_in_mem and assign\n");
    return mlir::failure();
  }
  for (const mlir::MemoryEffects::EffectInstance &effect : *effects) {
    mlir::Value affected = effect.getValue();
    if (!affected ||
        aliasAnalysis.getModRef(evalInMem.getBody(), affected).isModOrRef())
      return mlir::failure();
  }

  rewriter.setInsertionPoint(assign);
  fir::FirOpBuilder builder(rewriter, evalInMem.getOperation());
  mlir::Value rawLhs = hlfir::genVariableRawAddress(loc, builder, lhs);
  hlfir::computeEvaluateOpIn(loc, builder, evalInMem, rawLhs);
  rewriter.eraseOp(assign);
  rewriter.eraseOp(destroy);
  rewriter.eraseOp(evalInMem);
  return mlir::success();
}

llvm::LogicalResult EvaluateIntoMemoryAssignBufferization::matchAndRewrite(
    hlfir::EvaluateInMemoryOp evalInMem,
    mlir::PatternRewriter &rewriter) const {
  if (mlir::succeeded(tryUsingAssignLhsDirectly(evalInMem, rewriter)))
    return mlir::success();
  // Rewrite to temp + as_expr here so that the assign + as_expr pattern can
  // kick-in for simple types and at least implement the assignment inline
  // instead of call Assign runtime.
  fir::FirOpBuilder builder(rewriter, evalInMem.getOperation());
  mlir::Location loc = evalInMem.getLoc();
  auto [temp, isHeapAllocated] = hlfir::computeEvaluateOpInNewTemp(
      loc, builder, evalInMem, evalInMem.getShape(), evalInMem.getTypeparams());
  rewriter.replaceOpWithNewOp<hlfir::AsExprOp>(
      evalInMem, temp, /*mustFree=*/builder.createBool(loc, isHeapAllocated));
  return mlir::success();
}

class OptimizedBufferizationPass
    : public hlfir::impl::OptimizedBufferizationBase<
          OptimizedBufferizationPass> {
public:
  void runOnOperation() override {
    mlir::MLIRContext *context = &getContext();

    mlir::GreedyRewriteConfig config;
    // Prevent the pattern driver from merging blocks
    config.setRegionSimplificationLevel(
        mlir::GreedySimplifyRegionLevel::Disabled);

    mlir::RewritePatternSet patterns(context);
    // TODO: right now the patterns are non-conflicting,
    // but it might be better to run this pass on hlfir.assign
    // operations and decide which transformation to apply
    // at one place (e.g. we may use some heuristics and
    // choose different optimization strategies).
    // This requires small code reordering in ElementalAssignBufferization.
    patterns.insert<ElementalAssignBufferization>(context);
    patterns.insert<BroadcastAssignBufferization>(context);
    patterns.insert<EvaluateIntoMemoryAssignBufferization>(context);

    if (mlir::failed(mlir::applyPatternsGreedily(
            getOperation(), std::move(patterns), config))) {
      mlir::emitError(getOperation()->getLoc(),
                      "failure in HLFIR optimized bufferization");
      signalPassFailure();
    }
  }
};
} // namespace
