#ifndef LLVM_PROJECT_OURCFG_H
#define LLVM_PROJECT_OURCFG_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;

class OurCFG {
    public:
        OurCFG(Function& F);
        void DFS(BasicBlock* BB);
        bool isReachable(BasicBlock* BB);

    private:
        std::string FunctionName;
        std::unordered_set<BasicBlock*> Visited;
        std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> AdjacencyList;
        void CreateCFG(Function& F);
};

#endif 
