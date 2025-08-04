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
    // Use variable table for user-defined processes
    int addr1, addr2;

    if (proc->variableTable.count(arg1)) {
        addr1 = proc->variableTable[arg1];
    }
    else {
        throw std::runtime_error("Variable " + arg1 + " not found");
    }

    if (proc->variableTable.count(arg2)) {
        addr2 = proc->variableTable[arg2];
    }
    else {
        throw std::runtime_error("Variable " + arg2 + " not found");
    }

    uint16_t val1 = MemoryManager::getInstance().read(proc, addr1);
    uint16_t val2 = MemoryManager::getInstance().read(proc, addr2);

    uint32_t temp = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    uint16_t result = static_cast<uint16_t>(std::min(temp, static_cast<uint32_t>(65535)));

    int resAddr = MemoryManager::getInstance().allocateVariable(proc, resultVar);
    MemoryManager::getInstance().write(proc, resAddr, static_cast<uint8_t>(result));

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