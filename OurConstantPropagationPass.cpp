#include "ConstantPropagationInstruction.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <unordered_set>

using namespace llvm;

namespace {
struct OurConstantPropagationPass : public FunctionPass {
    std::vector<Value*> Variables;
    std::vector<ConstantPropagationInstruction*> Instructions;

    static char ID;
    OurConstantPropagationPass() : FunctionPass(ID) {}

    void findAllInstructions(Function& F) {
        for (BasicBlock& BB : F) {
            for (Instruction& I : BB) {
                ConstantPropagationInstruction* CPI = new ConstantPropagationInstruction(&I, Variables);
                Instructions.push_back(CPI);

                Instruction* Previous = I.getPrevNonDebugInstruction(); // vraca prethodnika trenutne instrukcije

                if (Previous == nullptr) {
                    for (BasicBlock* Pred : predecessors(&BB)) {
                        Instruction* Terminator = Pred->getTerminator();
                        CPI->addPredecessor(*std::find_if(Instructions.begin(), Instructions.end(),
                                                          [Terminator](ConstantPropagationInstruction* CPI)
                                                          { return CPI->getInstruction() == Terminator; }));
                    }
                }
                else {
                    CPI->addPredecessor(Instructions[Instructions.size() - 2]);
                }
            }
        }
    }

    void findAllVariables(Function& F) {
        for (BasicBlock& BB : F) {
            for (Instruction& I : BB) {
                if (isa<AllocaInst>(&I)) {
                    Variables.push_back(&I);
                }
            }
        }
    }

    void setStatusForFirstInstruction() {
        for (Value* Variable : Variables) {
            Instructions.front()->setStatusBefore(Variable, Top);
        }
    }

    bool checkRuleOne(ConstantPropagationInstruction* CPI, Value* Variable) {
        for (ConstantPropagationInstruction* Predecessor : CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Top) {
                return CPI->getStatusBefore(Variable) == Top;
            }
        }

        return true;
    }

    void applyRuleOne(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusBefore(Variable, Top);
    }

    bool checkRuleTwo(ConstantPropagationInstruction* CPI, Value* Variable) {
        std::unordered_set<int> Values;

        for (ConstantPropagationInstruction* Predecessor : CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Const) {
                Values.insert(Predecessor->getValueAfter(Variable));
            }
        }

        if (Values.size() > 1) {
            return CPI->getStatusBefore(Variable) == Top;
        }

        return true;
    }

    void applyRuleTwo(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusBefore(Variable, Top);
    }

    bool checkRuleThree(ConstantPropagationInstruction* CPI, Value* Variable) {
        std::unordered_set<int> Values;

        for (ConstantPropagationInstruction* Predecessor : CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Const) {
                Values.insert(Predecessor->getValueAfter(Variable));
            }
            if (Predecessor->getStatusAfter(Variable) == Top) {
                return true;
            }
        }

        if (Values.size() == 1) {
            return CPI->getStatusBefore(Variable) == Const &&
                   CPI->getValueBefore(Variable) == *Values.begin();
        }

        return true;
    }

    void applyRuleThree(ConstantPropagationInstruction* CPI, Value* Variable, int Value) {
        CPI->setStatusBefore(Variable, Const, Value);
    }

    bool checkRuleFour(ConstantPropagationInstruction* CPI, Value* Variable) {
        for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Top || Predecessor->getStatusAfter(Variable) == Const) {
                return true;
            }
        }

        // Nalazimo se u prvoj instrukciji u funkciji
        if (CPI->getPredecessors().size() == 0) {
            return true;
        }

        return CPI->getStatusBefore(Variable) == Bottom;
    }

    void applyRuleFour(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusBefore(Variable, Bottom);
    }

    bool checkRuleFive(ConstantPropagationInstruction* CPI, Value* Variable) {
        if (CPI->getStatusBefore(Variable) == Bottom) {
            return CPI->getStatusAfter(Variable) == Bottom;
        }

        return true;
    }

    void applyRuleFive(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusAfter(Variable, Bottom);
    }

    bool checkRuleSix(ConstantPropagationInstruction* CPI, Value* Variable) {
        Instruction* Instr = CPI->getInstruction();
        if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable) {
            if (ConstantInt* ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(0))) {
                return CPI->getStatusAfter(Variable) == Const && CPI->getValueAfter(Variable) == ConstInt->getSExtValue();
            }
        }

        return true;
    }

    void applyRuleSix(ConstantPropagationInstruction* CPI, Value* Variable, int Value) {
        CPI->setStatusAfter(Variable, Const, Value);
    }

    bool checkRuleSeven(ConstantPropagationInstruction* CPI, Value* Variable) {
        Instruction* Instr = CPI->getInstruction();
        if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable && !isa<ConstantInt>(Instr->getOperand(0))) {
            return CPI->getStatusAfter(Variable) == Top;
        }

        return true;
    }

    void applyRuleSeven(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusAfter(Variable, Top);
    }

    bool checkRuleEight(ConstantPropagationInstruction* CPI, Value* Variable) {
        if (isa<StoreInst>(CPI->getInstruction()) && CPI->getInstruction()->getOperand(1) == Variable) {
            return true;
        }

        return CPI->getStatusBefore(Variable) == CPI->getStatusAfter(Variable);
    }

    void applyRuleEight(ConstantPropagationInstruction* CPI, Value* Variable) {
        CPI->setStatusAfter(Variable, CPI->getStatusBefore(Variable), CPI->getValueBefore(Variable));
    }

    void propagateVariable(Value* Variable) {
        bool RuleApplied;

        while(true) {

            RuleApplied = false;

            for (ConstantPropagationInstruction* CPI : Instructions) {
                if (!checkRuleOne(CPI, Variable)) {
                    applyRuleOne(CPI, Variable);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleTwo(CPI, Variable)) {
                    applyRuleTwo(CPI, Variable);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleThree(CPI, Variable)) {
                    int Value;

                    for (ConstantPropagationInstruction* Predecessor : CPI->getPredecessors()) {
                        if (Predecessor->getStatusAfter(Variable) == Const) {
                            Value = Predecessor->getValueAfter(Variable);
                            break;
                        }
                    }

                    applyRuleThree(CPI, Variable, Value);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleFour(CPI, Variable)) {
                    applyRuleFour(CPI, Variable);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleFive(CPI, Variable)) {
                    applyRuleFive(CPI, Variable);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleSix(CPI, Variable)) {
                    ConstantInt* ConstInt = dyn_cast<ConstantInt>(CPI->getInstruction()->getOperand(0));
                    applyRuleSix(CPI, Variable, ConstInt->getSExtValue());
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleSeven(CPI, Variable)) {
                    applyRuleSeven(CPI, Variable);
                    RuleApplied = true;
                    break;
                }

                if (!checkRuleEight(CPI, Variable)) {
                    applyRuleEight(CPI, Variable);
                    RuleApplied = true;
                    break;
                }
            }

            if (!RuleApplied) {
                break;
            }
        }
    }

    void runAlgorithm() {
        for (Value* Variable : Variables) {
            propagateVariable(Variable);
        }
    }

    void modifyIR() {
        std::unordered_map<Value*, Value*> VariablesMap;

        for (ConstantPropagationInstruction* CPI : Instructions) {
            if (isa<LoadInst>(CPI->getInstruction())) {
                VariablesMap[CPI->getInstruction()] = CPI->getInstruction()->getOperand(0);
            }
        }

        for (ConstantPropagationInstruction* CPI : Instructions) {
            Instruction* Instr = CPI->getInstruction();

            if (isa<StoreInst>(Instr)) {
                Value* Operand = Instr->getOperand(0);
                if (CPI->getStatusBefore(VariablesMap[Operand]) == Const) {
                    int Value = CPI->getValueBefore(VariablesMap[Operand]);
                    ConstantInt* ConstInt = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), Value);
                    Operand->replaceAllUsesWith(ConstInt);
                }
            } else if (isa<BinaryOperator>(Instr) || isa<ICmpInst>(Instr)) {
                Value* Lhs = Instr->getOperand(0); Value* Rhs = Instr->getOperand(1);
                Value* VarLHS = VariablesMap[Lhs]; Value* VarRHS = VariablesMap[Rhs];

                if (VarLHS != nullptr && CPI->getStatusBefore(VarLHS) == Const) {
                    Lhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarLHS)));
                }

                if (VarRHS != nullptr && CPI->getStatusBefore(VarRHS) == Const) {
                    Rhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarRHS)));
                }
            }
        }
    }

    bool runOnFunction(Function& F) override {
        findAllVariables(F);
        findAllInstructions(F);
        setStatusForFirstInstruction();
        runAlgorithm();
        modifyIR();

        return true;
    }
};
} 

char OurConstantPropagationPass::ID = 0;
static RegisterPass<OurConstantPropagationPass> X("our-constant-propagation", "Our simple constant propagation pass", false, false);