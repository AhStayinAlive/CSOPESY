#include "WriteInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <sstream>

WriteInstruction::WriteInstruction(int addr, const std::string& varName, const std::string& logPrefix)
    : address(addr), variableName(varName), logPrefix(logPrefix) {
}

void WriteInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Use variable table
    int varAddr;

    if (proc->variableTable.count(variableName)) {
        varAddr = proc->variableTable[variableName];
    }
    else {
        throw std::runtime_error("Variable " + variableName + " not found");
    }

    // Read 16-bit value
    uint8_t lowByte = MemoryManager::getInstance().read(proc, varAddr);
    uint8_t highByte = 0;
    if (varAddr + 1 < proc->virtualMemoryLimit) {
        highByte = MemoryManager::getInstance().read(proc, varAddr + 1);
    }
    uint16_t value = lowByte | (static_cast<uint16_t>(highByte) << 8);

    // Validate address is within bounds
    if (address < 0 || address >= proc->virtualMemoryLimit) {
        throw std::runtime_error("Write address " + std::to_string(address) + " out of bounds");
    }

    // Write to specified address (just the low byte for now, can be expanded)
    MemoryManager::getInstance().write(proc, address, static_cast<uint8_t>(value & 0xFF));

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | WRITE: " << value << " to 0x" << std::hex << address;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | WRITE: " << value << " to 0x" << std::hex << address;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}