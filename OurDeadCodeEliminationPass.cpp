#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"


#include "OurCFG.h" 

using namespace llvm;

namespace {
struct OurDeadCodeEliminationPass : public FunctionPass {

    std::unordered_map<Value*, bool> Variables;
    std::unordered_map<Value*, Value*> VariablesMap;
    std::vector<Instruction*> InstructionsToRemove;
    bool InstructionRemoved;

    static char ID;
    OurDeadCodeEliminationPass() : FunctionPass(ID) {}

    void handleOperand(Value* Operand) {
        if (Variables.find(Operand) != Variables.end()) {
            Variables[Operand] = true;
        }
        if (VariablesMap.find(Operand) != VariablesMap.end()) {
            Variables[VariablesMap[Operand]] = true;
        }
    }

    void eliminateDeadInstructions(Function& F) {
        InstructionsToRemove.clear();

        for (BasicBlock& BB : F) {
            for (Instruction& I : BB) {
                if (!I.getType()->isVoidTy() && !isa<CallInst>(&I)) {
                    Variables[&I] = false;
                }

                if (isa<LoadInst>(&I)) {
                    VariablesMap[&I] = I.getOperand(0);
                }

                if (isa<StoreInst>(&I)) {
                    handleOperand(I.getOperand(0));
                } else {
                    for (size_t i = 0; i < I.getNumOperands(); i++) {
                        handleOperand(I.getOperand(i));
                    }
                }
            }
        }

        for (BasicBlock& BB : F) {
            for (Instruction& I : BB) {
                if (isa<StoreInst>(&I)) {
                    if (!Variables[I.getOperand(1)]) {
                        InstructionsToRemove.push_back(&I);
                    }
                } else {
                    if (Variables.find(&I) != Variables.end() && !Variables[&I]) {
                        InstructionsToRemove.push_back(&I);
                    }
                }
            }
        }

        if (InstructionsToRemove.size() > 0) {
            InstructionRemoved = true;
        }

        for (Instruction* Instr : InstructionsToRemove) {
            Instr->eraseFromParent();
        }
    }

    void eliminateUnreachableInstructions(Function &F) {
        std::vector<BasicBlock*> UnreachableBlocks;

        OurCFG* CFG = new OurCFG(F);
        CFG->DFS(&F.front());
 
        for (BasicBlock& BB : F) {
            if (!CFG->isReachable(&BB)) {
                UnreachableBlocks.push_back(&BB);
            }
        }

        for (BasicBlock* BB : UnreachableBlocks) {
            BB->eraseFromParent();
        }
    }

    bool runOnFunction(Function& F) override {
        do {
            InstructionRemoved = false;
            eliminateDeadInstructions(F);
            eliminateUnreachableInstructions(F);
        } while (InstructionRemoved);

        return true;
    }
};
} 

char OurDeadCodeEliminationPass::ID = 0;
static RegisterPass<OurDeadCodeEliminationPass> X("dead-code-elimination", "Our simple constant folding", false, false);