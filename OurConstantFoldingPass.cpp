#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace llvm;

namespace {
struct OurConstantFoldingPass : public FunctionPass {
    
    std::vector<Instruction*> InstructionsToRemove;

    static char ID;
    OurConstantFoldingPass() : FunctionPass(ID) {}

    void handleBinaryOperator(Instruction& I) {
        Value* Lhs = I.getOperand(0); Value* Rhs = I.getOperand(1);

        ConstantInt* LhsValue; ConstantInt* RhsValue;
        int Value;

        if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
            return;
        }

        if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
            return;
        }

        if (isa<AddOperator>(&I)) {
            Value = LhsValue->getSExtValue() + RhsValue->getSExtValue();
        } else if (isa<SubOperator>(&I)) {
            Value = LhsValue->getSExtValue() - RhsValue->getSExtValue();
        } else if (isa<MulOperator>(&I)) {
            Value = LhsValue->getSExtValue() * RhsValue->getSExtValue();
        } else if (isa<SDivOperator>(&I)) {
            if (RhsValue->getSExtValue() == 0) {
                errs() << "Division by zero not allowed!\n";
                exit(1);
            }

            Value = LhsValue->getSExtValue() / RhsValue->getSExtValue();
        }

        I.replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(I.getContext()), Value));
    }

    void handleCompareInstruction(Instruction &I) {
        Value* Lhs = I.getOperand(0); Value* Rhs = I.getOperand(1);
        ConstantInt* LhsValue; ConstantInt* RhsValue;
        bool Value;

        if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
            return;
        }

        if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
            return;
        }

        ICmpInst* Cmp = dyn_cast<ICmpInst>(&I);
        auto Pred = Cmp->getSignedPredicate();

        if (Pred == ICmpInst::ICMP_EQ) {
            Value = LhsValue == RhsValue;
        } else if (Pred == ICmpInst::ICMP_NE) {
            Value = LhsValue != RhsValue;
        } else if (Pred == ICmpInst::ICMP_SGT) {
            Value = LhsValue > RhsValue;
        } else if (Pred == ICmpInst::ICMP_SLT) {
            Value = LhsValue < RhsValue;
        } else if (Pred == ICmpInst::ICMP_SGE) {
            Value = LhsValue >= RhsValue;
        } else if (Pred == ICmpInst::ICMP_SLE) {
            Value = LhsValue <= RhsValue;
        }

        I.replaceAllUsesWith(ConstantInt::get(Type::getInt1Ty(I.getContext()), Value));
    }

    void handleBranchInstruction(Instruction& I) {
        BranchInst* BranchInstr = dyn_cast<BranchInst>(&I);

        if (BranchInstr->isConditional()) {
            ConstantInt* Condition = dyn_cast<ConstantInt>(BranchInstr->getCondition());

            if (Condition == nullptr) {
                return;
            }

            if (Condition->getZExtValue() == 1) {
                BranchInst::Create(BranchInstr->getSuccessor(0), BranchInstr->getParent());
            } else {
                BranchInst::Create(BranchInstr->getSuccessor(1), BranchInstr->getParent());
            }

            InstructionsToRemove.push_back(&I);
        }
    }

    void iterateInstructions(Function& F) {
        for (BasicBlock& BB : F) {
            for (Instruction& I : BB) {
                if (isa<BinaryOperator>(&I)) {
                    handleBinaryOperator(I);
                } else if (isa<ICmpInst>(&I)) {
                    handleCompareInstruction(I);
                } else if (isa<BranchInst>(&I)) {
                    handleBranchInstruction(I);
                }
            }
        }

        for (Instruction* Instr : InstructionsToRemove) {
            Instr->eraseFromParent();
        }
    }

    bool runOnFunction(Function& F) override {
        iterateInstructions(F);

        return true;
    }
};
} 

char OurConstantFoldingPass::ID = 0;
static RegisterPass<OurConstantFoldingPass> X("constant-folding", "Our simple constant folding", false, false);