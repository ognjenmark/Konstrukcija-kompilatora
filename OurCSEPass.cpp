#include "llvm/IR/Function.h"                 
#include "llvm/IR/Instruction.h"              
#include "llvm/IR/Instructions.h"             
#include "llvm/Pass.h"                        
#include "llvm/IR/Dominators.h"               
#include "llvm/IR/Operator.h"                 
#include "llvm/Support/raw_ostream.h"         

#include <string>
#include <vector>
#include <unordered_map>                      

using namespace llvm;

namespace {
struct OurCSEPass : public FunctionPass {
    static char ID;                          
    OurCSEPass() : FunctionPass(ID) {}       

    // Za svaki pokazivač pamti poslednju load instrukciju koja je učitana sa te adrese
    std::unordered_map<Value*, Instruction*> LastSeenLoadInstruction;

    // Pamti izraze (kao string) i instrukciju koja ih je izračunala
    std::unordered_map<std::string, Instruction*> ExpressionMap;

    std::vector<Instruction*> InstructionsToRemove;

    bool runOnFunction(Function& F) override {

        // Dohvata dominator stablo za funkciju
        DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

        bool Changed = false;               

        for(BasicBlock& BB : F) {
            for(Instruction& I : BB) {
                if(LoadInst* LoadInstr = dyn_cast<LoadInst>(&I)) {
                    // Pokazivač sa kog se učitava vrednost
                    Value* PointerOperand = LoadInstr->getPointerOperand();

                    // Da li već postoji load za ovaj pokazivač
                    if(LastSeenLoadInstruction.find(PointerOperand) != LastSeenLoadInstruction.end()) {
                        // Prethodna load instrukciju za isti pokazivač
                        Instruction* PreviousInstr = LastSeenLoadInstruction[PointerOperand];

                        if(DT.dominates(PreviousInstr, LoadInstr)) {
                            LoadInstr->replaceAllUsesWith(PreviousInstr);
                            InstructionsToRemove.push_back(&I);
                            Changed = true;
                        }
                    }
                    // Pamti da je ova load instrukcija poslednja viđena za ovaj pokazivač
                    LastSeenLoadInstruction[PointerOperand] = &I;

                } else if(StoreInst* StoreInstr = dyn_cast<StoreInst>(&I)) {
                    // Dohvata pokazivač na koji se piše
                    Value* PointerOperand = StoreInstr->getPointerOperand();

                    // Store menja memorijski sadržaj pa prethodni load za taj pokazivač nije validan
                    if(LastSeenLoadInstruction.find(PointerOperand) != LastSeenLoadInstruction.end()) {
                        // Ukloni prethodni load iz mape jer je memorija promenjena
                        LastSeenLoadInstruction.erase(PointerOperand);
                    }

                } else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)) {
                    Value* LeftOp = BO->getOperand(0);
                    Value* RightOp = BO->getOperand(1);
                    
                    // Ako je operator komutativan sortira operande po adresi da dobijemo jedinstveni ključ
                    if(BO->isCommutative() && LeftOp > RightOp) {
                        std::swap(LeftOp, RightOp);
                    }

                    std::string Key;                  // String koji će predstavljati jedinstveni izraz
                    raw_string_ostream RSO(Key);     // Strim za lako konstruisanje stringa

                    RSO << BO->getOpcodeName() << " ";
                    LeftOp->print(RSO);
                    RSO << ", ";                     
                    RightOp->print(RSO);

                    // Konvertuj strim u string ključ
                    std::string ExpressionKey = RSO.str();

                    // Proveri da li smo već videli isti izraz (isti operator i isti operandi)
                    if(ExpressionMap.find(ExpressionKey) != ExpressionMap.end()) {
                        // Uzima prethodnu instrukciju koja je izračunala ovaj izraz
                        Instruction* PreviousInstr = ExpressionMap[ExpressionKey];

                        // Proveri da li prethodna instrukcija dominira trenutnu (da je prethodna uvek ranije)
                        if(DT.dominates(PreviousInstr, BO)) {
                            BO->replaceAllUsesWith(PreviousInstr);
                            InstructionsToRemove.push_back(&I);
                            Changed = true;
                        } 
                    } else {
                        // Ako izraz nije viđen do sada, pamti ga u mapi
                        ExpressionMap[ExpressionKey] = &I;
                    }
                }
            }
        }

        for(Instruction* I : InstructionsToRemove) {
            I->eraseFromParent();
        }
        return Changed;                       
    }

    // Obezbeđuje da se dominator stablo izračuna pre našeg passa i da pass ne menja CFG
    void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.setPreservesCFG();
    }
};
} 

char OurCSEPass::ID = 0;
static RegisterPass<OurCSEPass> X("cse-pass", "Our CSE Pass", false, false);
