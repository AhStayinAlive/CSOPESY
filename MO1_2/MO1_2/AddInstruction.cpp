// AddInstruction.cpp
#include "AddInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

AddInstruction::AddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    int maxSafeAddress = std::max(1, proc->virtualMemoryLimit - static_cast<int>(sizeof(uint16_t)));
    int addr1 = std::hash<std::string>{}(arg1) % maxSafeAddress;
    int addr2 = std::hash<std::string>{}(arg2) % maxSafeAddress;

    uint16_t val1 = MemoryManager::getInstance().read(proc, addr1);
    uint16_t val2 = MemoryManager::getInstance().read(proc, addr2);
    uint32_t temp = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    uint16_t result = static_cast<uint16_t>(std::min(temp, static_cast<uint32_t>(65535)));

    int resAddr = std::hash<std::string>{}(resultVar) % maxSafeAddress;
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


