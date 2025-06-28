#include "ForInstruction.h"
#include "utils.h"
#include "process.h"
#include <sstream>
#include <thread>
#include <chrono>

ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& subs)
    : loopCount(count), subInstructions(subs) {}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId, int currentDepth) {
    for (int i = 0; i < loopCount; ++i) {
        for (const auto& subInstr : subInstructions) {
            subInstr->execute(proc, coreId, currentDepth + 1);  // recursive execution with increased depth
        }
    }

    // Optionally, log when FOR completes
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | FOR completed " << loopCount << " times at depth " << currentDepth;
    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
