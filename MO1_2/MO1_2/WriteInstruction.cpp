#include "WriteInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <sstream>
#include <iomanip>

WriteInstruction::WriteInstruction(int addr, const std::string& varName, const std::string& logPrefix)
    : address(addr), variableName(varName), logPrefix(logPrefix) {
}

void WriteInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    //// Validate address bounds
    //if (address < 0 || address >= proc->virtualMemoryLimit) {
    //    std::ostringstream oss;
    //    oss << "Memory access violation: WRITE at address 0x" << std::hex << std::uppercase << address
    //        << " out of bounds [0, 0x" << std::hex << std::uppercase << (proc->virtualMemoryLimit - 1) << "]";
    //    throw std::runtime_error(oss.str());
    //}

    uint16_t value = 0;

    // Check both memory locations and use the most recent value
    if (proc->memory.count(variableName)) {
        value = proc->memory[variableName];
    }
    if (proc->variableTable.count(variableName)) {
        // Also check variable table for potentially newer value
        int varAddr = proc->variableTable[variableName];
        uint8_t lowByte = MemoryManager::getInstance().read(proc, varAddr);
        uint8_t highByte = 0;
        if (varAddr + 1 < proc->virtualMemoryLimit) {
            highByte = MemoryManager::getInstance().read(proc, varAddr + 1);
        }
        value = lowByte | (static_cast<uint16_t>(highByte) << 8);
    }
    else {
        throw std::runtime_error("WRITE: Variable '" + variableName + "' not found");
    }

    // Clamp value to uint16 range
    if (value > 65535) value = 65535;

    // Write 16-bit value to specified memory address (as 2 bytes)
    MemoryManager::getInstance().write(proc, address, static_cast<uint8_t>(value & 0xFF));
    if (address + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, address + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | WRITE: " << value
            << " to 0x" << std::hex << std::uppercase << address << " - SUCCESS";
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | WRITE: " << value
            << " to 0x" << std::hex << std::uppercase << address << " - SUCCESS";
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}