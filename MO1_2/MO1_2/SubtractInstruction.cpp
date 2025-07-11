﻿#include "SubtractInstruction.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

SubtractInstruction::SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void SubtractInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    if (proc->memory.find(arg1) == proc->memory.end()) proc->memory[arg1] = 0;
    if (proc->memory.find(arg2) == proc->memory.end()) proc->memory[arg2] = 0;

    uint16_t val1 = proc->memory[arg1];
    uint16_t val2 = proc->memory[arg2];

    uint16_t result = (val1 > val2) ? (val1 - val2) : 0;
    proc->memory[resultVar] = result;

    std::ostringstream logEntry;
    if (logPrefix != "") {
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
