#include "llvm/IR/BasicBlock.h"
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
#include <unordered_set>

using namespace llvm;

namespace {

struct DeadBasicBlockEliminationPass : public FunctionPass {
    static char ID;
    DeadBasicBlockEliminationPass() : FunctionPass(ID) {}

    // Skup svih dostižnih blokova koje DFS otkrije
    std::unordered_set<BasicBlock*> ReachableBlocks;

    // Lista blokova koji treba da se obrišu
    std::vector<BasicBlock*> BlocksToRemove;

    // DFS za pronalaženje dostižnih blokova
    void DFS(BasicBlock* Current) {
        if(ReachableBlocks.find(Current) != ReachableBlocks.end()) {
            return;
        }
        ReachableBlocks.insert(Current);
        for (BasicBlock* Successors : successors(Current)) {
            DFS(Successors); // Rekurzivno posećuj naslednike
        }
    }

    bool runOnFunction(Function& F) override {
        DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

        ReachableBlocks.clear();
        BlocksToRemove.clear();

        DFS(&F.getEntryBlock());

        for (BasicBlock& BB : F) {
            // Ako blok nije dostižan ili ga entry ne dominira planiramo ga za brisanje
            if (ReachableBlocks.find(&BB) == ReachableBlocks.end() || !DT.dominates(&F.getEntryBlock(), &BB)) {
                BlocksToRemove.push_back(&BB);
            }
        }

        if (BlocksToRemove.empty()) {
            return false;
        }

        for (BasicBlock* BB : BlocksToRemove) {
            // Prvo brišemo sve instrukcije u bloku unazad 
            while (!BB->empty()) {
                Instruction& I = BB->back(); 
                I.dropAllReferences();       
                I.eraseFromParent();        
            }
            BB->eraseFromParent();
        }

        return true;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
        AU.addRequired<DominatorTreeWrapperPass>();
        // AU.setPreservesCFG(); 
    }
};

} 

char DeadBasicBlockEliminationPass::ID = 0;
static RegisterPass<DeadBasicBlockEliminationPass> X("dead-basic-block-elim", "Our Dead Basic Block Elimination Pass", false, false);
