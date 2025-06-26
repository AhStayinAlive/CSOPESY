#include "DeclareInstruction.h"
#include "utils.h"

DeclareInstruction::DeclareInstruction(const std::string& var, uint16_t val)
    : varName(var), value(val) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[varName] = value;
    logToFile(proc->name, "DECLARE " + varName + " = " + std::to_string(value), coreId);
}
