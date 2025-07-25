#include "DeclareInstruction.h"
#include "process.h"
#include "utils.h"
#include <sstream>

DeclareInstruction::DeclareInstruction(const std::string& varName, int val, const std::string& logPrefix)
    : variableName(varName), value(val), logPrefix(logPrefix) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[variableName] = value;
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | DECLARE: " << variableName << " = " << value;
    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
