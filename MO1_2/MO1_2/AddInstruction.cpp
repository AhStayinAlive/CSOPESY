// AddInstruction.cpp
#include "AddInstruction.h"
#include "utils.h"

AddInstruction::AddInstruction(const std::string& res, const std::string& l, const std::string& r)
    : resultName(res), lhs(l), rhs(r) {
}

void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    uint16_t val1 = proc->memory.count(lhs) ? proc->memory[lhs] : std::stoi(lhs);
    uint16_t val2 = proc->memory.count(rhs) ? proc->memory[rhs] : std::stoi(rhs);
    proc->memory[resultName] = val1 + val2;
    logToFile(proc->name, "ADD " + resultName + " = " + std::to_string(val1) + " + " + std::to_string(val2), coreId);
}
