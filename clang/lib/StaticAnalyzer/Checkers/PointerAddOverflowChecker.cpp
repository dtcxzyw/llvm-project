//=== PointerAddOverflowChecker.cpp - Pointer add checker ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This files defines PointerAddOverflowChecker, a builtin checker that checks
// for pointer addition with negative size_t offsets.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/OperationKinds.h"
#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

using namespace clang;
using namespace ento;

namespace {
class PointerAddOverflowChecker
    : public Checker<check::PreStmt<BinaryOperator>,
                     check::PreStmt<ArraySubscriptExpr>> {
  const BugType BT_pointerAdd{this, "Dangerous pointer addition"};

public:
  void checkPreStmt(const BinaryOperator *BOp, CheckerContext &C) const;
  void checkPreStmt(const ArraySubscriptExpr *SubExpr, CheckerContext &C) const;
  void checkOffset(const Expr *E, CheckerContext &C) const;
};
} // end namespace

void PointerAddOverflowChecker::checkOffset(const Expr *E,
                                            CheckerContext &C) const {
  ASTContext &Ctx = C.getASTContext();
  QualType OffsetTy = E->getType();
  if (!OffsetTy->isUnsignedIntegerType())
    return;
  if (Ctx.getTypeSize(OffsetTy) != Ctx.getTypeSize(Ctx.getSizeType()))
    return;
  auto *BO = llvm::dyn_cast<BinaryOperator>(E->IgnoreParenImpCasts());
  if (!BO)
    return;
  QualType LHSTy = BO->getLHS()->IgnoreImpCasts()->getType();
  QualType RHSTy = BO->getRHS()->IgnoreImpCasts()->getType();

  // Make sure integer promotions are involved.
  if (!(LHSTy->isUnsignedIntegerType() && RHSTy->isSignedIntegerType()) &&
      !(LHSTy->isSignedIntegerType() && RHSTy->isUnsignedIntegerType()))
    return;

  ProgramStateRef State = C.getState();
  SVal Offset = C.getSVal(E);
  if (auto N = Offset.getAs<NonLoc>()) {
    unsigned BitWidth = Ctx.getTypeSize(OffsetTy);
    ConstraintManager &CM = C.getConstraintManager();
    auto [NonNeg, Neg] = CM.assumeInclusiveRangeDual(State, *N, llvm::APSInt(llvm::APInt::getZero(BitWidth)), llvm::APSInt(llvm::APSInt::getSignedMaxValue(BitWidth)));
    if (!NonNeg || Neg) {
      if (ExplodedNode *N = C.generateNonFatalErrorNode()) {
        constexpr llvm::StringLiteral Msg =
            "Pointer addition with a negative size_t offset is dangerous because "
            "it is likely to overflow. Convert the offset to a signed type "
            "instead.";
        auto R = std::make_unique<PathSensitiveBugReport>(BT_pointerAdd, Msg, N);
        R->addRange(E->getSourceRange());
        C.emitReport(std::move(R));
      }
    }
  }
}

void PointerAddOverflowChecker::checkPreStmt(const ArraySubscriptExpr *SubsExpr,
                                             CheckerContext &C) const {
  if (!SubsExpr->getBase()->getType()->isPointerType())
    return;
  return checkOffset(SubsExpr->getIdx(), C);
}

void PointerAddOverflowChecker::checkPreStmt(const BinaryOperator *BOp,
                                             CheckerContext &C) const {
  BinaryOperatorKind OpKind = BOp->getOpcode();
  if (OpKind != BO_AddAssign && OpKind != BO_Add)
    return;

  const Expr *Lhs = BOp->getLHS();
  const Expr *Rhs = BOp->getRHS();
  ProgramStateRef State = C.getState();

  if (Rhs->getType()->isIntegerType() && Lhs->getType()->isPointerType())
    checkOffset(Rhs, C);
  else if (Lhs->getType()->isIntegerType() && Rhs->getType()->isPointerType())
    checkOffset(Lhs, C);
}

void ento::registerPointerAddOverflowChecker(CheckerManager &mgr) {
  mgr.registerChecker<PointerAddOverflowChecker>();
}

bool ento::shouldRegisterPointerAddOverflowChecker(const CheckerManager &mgr) {
  return !mgr.getLangOpts().PointerOverflowDefined;
}
