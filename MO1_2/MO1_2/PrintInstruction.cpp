// PrintInstruction.cpp
#include "PrintInstruction.h"
#include "utils.h"
#include "process.h"
PrintInstruction::PrintInstruction(const std::string& msg) : message(msg) {}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    logToFile(proc->name, "PRINT: " + message, coreId);
}