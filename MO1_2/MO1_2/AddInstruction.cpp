// AddInstruction.cpp
#include "AddInstruction.h"
#include "utils.h"
#include <stdexcept>
#include "process.h"
AddInstruction::AddInstruction(const std::string& res, const std::string& l, const std::string& r)
    : resultName(res), lhs(l), rhs(r) {
}

void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto lhsIt = proc->memory.find(lhs);
    auto rhsIt = proc->memory.find(rhs);

    if (lhsIt == proc->memory.end() || rhsIt == proc->memory.end()) {
        throw std::runtime_error("Variable not found in ADD operation");
    }

    proc->memory[resultName] = lhsIt->second + rhsIt->second;
    logToFile(proc->name, "ADD " + resultName + " = " + lhs + " + " + rhs, coreId);
}