#include "AddInstruction.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <cstdint>
#include <sstream>

AddInstruction::AddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs)
    : resultVar(result), arg1(lhs), arg2(rhs) {
}


void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Auto-declare missing variables with value 0
    if (proc->memory.find(arg1) == proc->memory.end()) {
        proc->memory[arg1] = 0;
    }
    if (proc->memory.find(arg2) == proc->memory.end()) {
        proc->memory[arg2] = 0;
    }

    // Get values from memory (not parsing as integers)
    uint16_t val1 = proc->memory[arg1];
    uint16_t val2 = proc->memory[arg2];

    // Perform addition with overflow protection
    uint32_t temp = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    uint16_t result = static_cast<uint16_t>(std::min(temp, static_cast<uint32_t>(65535)));

    // Store result
    proc->memory[resultVar] = result;

    // Create log entry
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | ADD: " << val1 << " + " << val2 << " = " << result;

    std::string finalLog = logEntry.str();
    proc->logs.push_back(finalLog);
    logToFile(proc->name, finalLog, proc->coreAssigned);
}