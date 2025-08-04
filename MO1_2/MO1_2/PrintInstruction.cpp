// PrintInstruction.cpp
#include "PrintInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <iostream>
#include <sstream>
#include <cstdint>

PrintInstruction::PrintInstruction(const std::string& msg, const std::string& logPrefix)
    : message(msg), hasVariable(false), logPrefix(logPrefix) {
}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string output = message;

    if (hasVariable) {
        int addr = std::hash<std::string>{}(variableName) % Config::getInstance().maxMemPerProc;
        uint16_t varValue = MemoryManager::getInstance().read(proc, addr);
        output += std::to_string(varValue);
    }

    std::ostringstream logEntry;

    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | PRINT: \"" << output << "\"";
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | PRINT: \"" << output << "\"";
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}