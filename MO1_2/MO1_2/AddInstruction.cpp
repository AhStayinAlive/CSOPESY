#include "AddInstruction.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

AddInstruction::AddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}


void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    if (proc->memory.find(arg1) == proc->memory.end()) {
        proc->memory[arg1] = 0;
    }
    if (proc->memory.find(arg2) == proc->memory.end()) {
        proc->memory[arg2] = 0;
    }

    uint16_t val1 = proc->memory[arg1];
    uint16_t val2 = proc->memory[arg2];

    uint32_t temp = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    uint16_t result = static_cast<uint16_t>(std::min(temp, static_cast<uint32_t>(65535)));

    proc->memory[resultVar] = result;

    std::ostringstream logEntry;
    if (logPrefix != "") {
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
