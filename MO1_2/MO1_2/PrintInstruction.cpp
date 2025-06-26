// PrintInstruction.cpp
#include "PrintInstruction.h"
#include "utils.h"

PrintInstruction::PrintInstruction(const std::string& msg, bool isVar)
    : message(msg), isVariable(isVar) {
}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string output = message;
    if (isVariable && proc->memory.count(message)) {
        output += " = " + std::to_string(proc->memory[message]);
    }
    logToFile(proc->name, output, coreId);
    proc->logs.push_back(output);
}