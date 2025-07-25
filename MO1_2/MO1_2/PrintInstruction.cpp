#include "PrintInstruction.h"
#include "process.h"
#include "utils.h"
#include <sstream>

PrintInstruction::PrintInstruction(const std::string& msg, const std::string& logPrefix)
    : message(msg), hasVariable(false), logPrefix(logPrefix) {
}

PrintInstruction::PrintInstruction(const std::string& messagePart, const std::string& variableName, bool withVariable, const std::string& logPrefix)
    : message(messagePart), variableName(variableName), hasVariable(withVariable), logPrefix(logPrefix) {
}

PrintInstruction::PrintInstruction(const std::string& messagePart, const std::string& variableName, const std::string& logPrefix)
    : message(messagePart), variableName(variableName), hasVariable(true), logPrefix(logPrefix) {
}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid << " | PRINT: ";

    if (hasVariable && proc->memory.find(variableName) != proc->memory.end()) {
        logEntry << message << proc->memory[variableName];
    }
    else {
        logEntry << message;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
