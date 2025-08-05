// PrintInstruction.cpp
#include "PrintInstruction.h"
#include "MemoryManager.h"
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
        // ✅ Fix hash calculation to be safe for 16-bit access
        int maxSafeAddress = std::max(1, proc->virtualMemoryLimit - static_cast<int>(sizeof(uint16_t)));
        int addr = std::hash<std::string>{}(variableName) % maxSafeAddress;

        uint8_t lowByte = MemoryManager::getInstance().read(proc, addr);
        uint8_t highByte = 0;
        if (addr + 1 < proc->virtualMemoryLimit) {
            highByte = MemoryManager::getInstance().read(proc, addr + 1);
        }
        uint16_t varValue = lowByte | (static_cast<uint16_t>(highByte) << 8);
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
