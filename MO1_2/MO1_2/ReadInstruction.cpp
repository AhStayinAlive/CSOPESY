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
    if (address < 0) {
        // only check for negative addresses
        std::ostringstream oss;
        oss << "Memory access violation: WRITE at negative address";
        throw std::runtime_error(oss.str());
    }

    uint8_t lowByte = MemoryManager::getInstance().read(proc, address);
    uint8_t highByte = 0;

    if (address + 1 < proc->virtualMemoryLimit) {
        highByte = MemoryManager::getInstance().read(proc, address + 1);
    }

    uint16_t value = lowByte | (static_cast<uint16_t>(highByte) << 8);

    int varAddr = MemoryManager::getInstance().allocateVariable(proc, variableName);

    MemoryManager::getInstance().write(proc, varAddr, static_cast<uint8_t>(value & 0xFF));
    if (varAddr + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, varAddr + 1, static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    proc->memory[variableName] = value;

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | READ: " << variableName << " = " << value
            << " from 0x" << std::hex << std::uppercase << address;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | READ: " << variableName << " = " << value
            << " from 0x" << std::hex << std::uppercase << address;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
