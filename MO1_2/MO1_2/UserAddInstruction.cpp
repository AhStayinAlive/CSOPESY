#include "UserAddInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

UserAddInstruction::UserAddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void UserAddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // FIXED: Look in both variableTable and legacy memory for better compatibility
    uint16_t val1 = 0, val2 = 0;

    // Get value for arg1
    if (proc->variableTable.count(arg1)) {
        int addr1 = proc->variableTable[arg1];
        uint8_t lowByte = MemoryManager::getInstance().read(proc, addr1);
        uint8_t highByte = 0;
        if (addr1 + 1 < proc->virtualMemoryLimit) {
            highByte = MemoryManager::getInstance().read(proc, addr1 + 1);
        }
        val1 = lowByte | (static_cast<uint16_t>(highByte) << 8);
    }
    else if (proc->memory.count(arg1)) {
        val1 = proc->memory[arg1];
    }
    else {
        throw std::runtime_error("Variable " + arg1 + " not found");
    }

    // Get value for arg2
    if (proc->variableTable.count(arg2)) {
        int addr2 = proc->variableTable[arg2];
        uint8_t lowByte = MemoryManager::getInstance().read(proc, addr2);
        uint8_t highByte = 0;
        if (addr2 + 1 < proc->virtualMemoryLimit) {
            highByte = MemoryManager::getInstance().read(proc, addr2 + 1);
        }
        val2 = lowByte | (static_cast<uint16_t>(highByte) << 8);
    }
    else if (proc->memory.count(arg2)) {
        val2 = proc->memory[arg2];
    }
    else {
        throw std::runtime_error("Variable " + arg2 + " not found");
    }

    // Perform addition
    uint32_t temp = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    uint16_t result = static_cast<uint16_t>(std::min(temp, static_cast<uint32_t>(65535)));

    // Store result in both legacy memory and proper memory management
    proc->memory[resultVar] = result;
    int resAddr = MemoryManager::getInstance().allocateVariable(proc, resultVar);
    MemoryManager::getInstance().write(proc, resAddr, static_cast<uint8_t>(result & 0xFF));
    if (resAddr + 1 < proc->virtualMemoryLimit) {
        MemoryManager::getInstance().write(proc, resAddr + 1, static_cast<uint8_t>((result >> 8) & 0xFF));
    }

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | ADD: " << val1 << " + " << val2 << " = " << result;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | ADD: " << val1 << " + " << val2 << " = " << result;
    }

    std::string finalLog = logEntry.str();
    proc->logs.push_back(finalLog);
    logToFile(proc->name, finalLog, proc->coreAssigned);
}