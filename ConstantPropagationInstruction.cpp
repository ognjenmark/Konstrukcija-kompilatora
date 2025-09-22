#include "ConstantPropagationInstruction.h"

ConstantPropagationInstruction::ConstantPropagationInstruction(Instruction* Instr, const std::vector<Value*>& Variables) {
    this->Instr = Instr;
    for (Value* Variable : Variables) {
        setStatusBefore(Variable, Bottom);
        setStatusAfter(Variable, Bottom);
    }
}

void ConstantPropagationInstruction::setStatusAfter(Value* Variable, Status S, int value) {
    StatusAfter[Variable] = {S, value};
}

void ConstantPropagationInstruction::setStatusBefore(Value* Variable, Status S, int value) {
    StatusBefore[Variable] = {S, value};
}

Status ConstantPropagationInstruction::getStatusAfter(Value* Variable) {
    return StatusAfter[Variable].first;
}

Status ConstantPropagationInstruction::getStatusBefore(Value* Variable) {
    return StatusBefore[Variable].first;
}

int ConstantPropagationInstruction::getValueBefore(Value* Variable) {
    return StatusBefore[Variable].second;
}

int ConstantPropagationInstruction::getValueAfter(Value* Variable) {
    return StatusAfter[Variable].second;
}

void ConstantPropagationInstruction::addPredecessor(ConstantPropagationInstruction* Predecessor) {
    Predecessors.push_back(Predecessor);
}

Instruction* ConstantPropagationInstruction::getInstruction() {
    return Instr;
}

std::vector<ConstantPropagationInstruction*> ConstantPropagationInstruction::getPredecessors() {
    return Predecessors;
}