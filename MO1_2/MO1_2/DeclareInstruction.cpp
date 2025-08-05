#include "DeclareInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <sstream>
#include <cstdint>


DeclareInstruction::DeclareInstruction(
    const std::string& varName, 
    int val, 
    const std::string& logPrefix)
    : variableName(varName), value(static_cast<uint16_t>(std::clamp(val, 0, 65535))), logPrefix(logPrefix) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[variableName] = value;

    // ✅ Use safe hash calculation for consistency
    int maxSafeAddress = std::max(1, proc->virtualMemoryLimit - static_cast<int>(sizeof(uint16_t)));
    int address = std::hash<std::string>{}(variableName) % maxSafeAddress;

    // Write 16-bit value as two bytes
    MemoryManager::getInstance().write(proc, address, static_cast<uint8_t>(value & 0xFF));
    if (address + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, address + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    std::ostringstream logEntry;
    if (logPrefix != "") {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | DECLARE: " << variableName << " = " << value;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | DECLARE: " << variableName << " = " << value;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), proc->coreAssigned);
}
