// DeclareInstruction.cpp
#include "DeclareInstruction.h"
#include "utils.h"
#include "process.h"
DeclareInstruction::DeclareInstruction(const std::string& name, int val)
    : varName(name), value(val) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[varName] = static_cast<uint16_t>(value);
    logToFile(proc->name, "DECLARE " + varName + " = " + std::to_string(value), coreId);
}
