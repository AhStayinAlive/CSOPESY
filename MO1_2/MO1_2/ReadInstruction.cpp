#include "ReadInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <sstream>
#include <iomanip>

ReadInstruction::ReadInstruction(const std::string& varName, int addr, const std::string& logPrefix)
    : variableName(varName), address(addr), logPrefix(logPrefix) {
}

void ReadInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    //// Validate address bounds
    //if (address < 0 || address >= proc->virtualMemoryLimit) {
    //    std::ostringstream oss;
    //    oss << "Memory access violation: READ at address 0x" << std::hex << std::uppercase << address
    //        << " out of bounds [0, 0x" << std::hex << std::uppercase << (proc->virtualMemoryLimit - 1) << "]";
    //    throw std::runtime_error(oss.str());
    //}

    // Read 16-bit value from memory (stored as 2 bytes)
    uint8_t lowByte = MemoryManager::getInstance().read(proc, address);
    uint8_t highByte = 0;

    // Check if we can read the high byte
    if (address + 1 < proc->virtualMemoryLimit) {
        highByte = MemoryManager::getInstance().read(proc, address + 1);
    }

    uint16_t value = lowByte | (static_cast<uint16_t>(highByte) << 8);

    // Store the variable in the process's variable table and memory
    int varAddr = MemoryManager::getInstance().allocateVariable(proc, variableName);

    // Write the 16-bit value as two bytes in memory
    MemoryManager::getInstance().write(proc, varAddr, static_cast<uint8_t>(value & 0xFF));
    if (varAddr + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, varAddr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    // Also store in legacy memory map for compatibility
    proc->memory[variableName] = value;

    // Debug: Verify the write was successful
    try {
        uint8_t testRead = MemoryManager::getInstance().read(proc, address);
        std::ostringstream debugLog;
        debugLog << "[DEBUG] Write verification: address 0x" << std::hex << address
            << " contains value " << std::dec << static_cast<int>(testRead);
        proc->logs.push_back(debugLog.str());
    }
    catch (const std::exception& e) {
        std::ostringstream errorLog;
        errorLog << "[ERROR] Write verification failed: " << e.what();
        proc->logs.push_back(errorLog.str());
    }

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | READ: " << variableName << " = " << value
            << " from 0x" << std::hex << std::uppercase << address << " - SUCCESS";
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | READ: " << variableName << " = " << value
            << " from 0x" << std::hex << std::uppercase << address << " - SUCCESS";
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}