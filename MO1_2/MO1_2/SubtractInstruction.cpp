#include "SubtractInstruction.h"
#include "process.h"
#include "utils.h"
#include "MemoryManager.h"
#include "config.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

SubtractInstruction::SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void SubtractInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    int addr1 = std::hash<std::string>{}(arg1) % proc->virtualMemoryLimit;;
    int addr2 = std::hash<std::string>{}(arg2) % proc->virtualMemoryLimit;;

    uint16_t val1 = MemoryManager::getInstance().read(proc, addr1);
    uint16_t val2 = MemoryManager::getInstance().read(proc, addr2);

    uint16_t result = (val1 > val2) ? (val1 - val2) : 0;

    int resAddr = MemoryManager::getInstance().allocateVariable(proc, resultVar);
    MemoryManager::getInstance().write(proc, resAddr, static_cast<uint8_t>(result));

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | SUBTRACT: " << val1 << " - " << val2 << " = " << result;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | SUBTRACT: " << val1 << " - " << val2 << " = " << result;
    }

    std::string finalLog = logEntry.str();
    proc->logs.push_back(finalLog);
    logToFile(proc->name, finalLog, coreId);
}
