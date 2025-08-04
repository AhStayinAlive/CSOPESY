#include "ReadInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <sstream>

ReadInstruction::ReadInstruction(const std::string& varName, int addr, const std::string& logPrefix)
    : variableName(varName), address(addr), logPrefix(logPrefix) {
}

void ReadInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    uint16_t value = MemoryManager::getInstance().read(proc, address);

    // Store the full 16-bit value, not just 8-bit
    int varAddr = MemoryManager::getInstance().allocateVariable(proc, variableName);

    // Write the full 16-bit value as two bytes
    MemoryManager::getInstance().write(proc, varAddr, static_cast<uint8_t>(value & 0xFF));
    if (varAddr + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, varAddr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | READ: " << variableName << " = " << value << " from 0x" << std::hex << address;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | READ: " << variableName << " = " << value << " from 0x" << std::hex << address;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}