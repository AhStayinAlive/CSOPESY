#include "SubtractInstruction.h"
#include "process.h"
#include "utils.h"
#include <sstream>

SubtractInstruction::SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {}

void SubtractInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    uint16_t val1 = proc->memory.count(arg1) ? proc->memory[arg1] : 0;
    uint16_t val2 = proc->memory.count(arg2) ? proc->memory[arg2] : 0;

    uint16_t result = (val1 > val2) ? (val1 - val2) : 0;
    proc->memory[resultVar] = result;

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
             << "Core " << coreId << " | PID " << proc->pid
             << " | SUBTRACT: " << val1 << " - " << val2 << " = " << result;

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
