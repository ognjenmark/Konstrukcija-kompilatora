#ifndef LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H
#define LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include <unordered_map>
#include <vector>

using namespace llvm;

enum Status {
    Top,
    Bottom,
    Const
};

class ConstantPropagationInstruction {
    public:
        ConstantPropagationInstruction(Instruction* I, const std::vector<Value*>& V);
        Status getStatusBefore(Value* V);
        Status getStatusAfter(Value* V);
        void setStatusBefore(Value* V, Status S, int value = -1);
        void setStatusAfter(Value* V, Status S, int value = -1);
        int getValueBefore(Value* V);
        int getValueAfter(Value* V);
        void addPredecessor(ConstantPropagationInstruction* CPI);
        std::vector<ConstantPropagationInstruction*> getPredecessors();
        Instruction* getInstruction();

    private:
        Instruction* Instr;
        std::unordered_map<Value*, std::pair<Status, int>> StatusBefore;
        std::unordered_map<Value*, std::pair<Status, int>> StatusAfter;
        std::vector<ConstantPropagationInstruction*> Predecessors;

};

#endif 