#include "SleepInstruction.h"
#include "process.h"
#include "utils.h"
#include <sstream>

SleepInstruction::SleepInstruction(int ms, const std::string& logPrefix)
    : duration(ms), logPrefix(logPrefix) {
}

void SleepInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->setWakeupTick(proc->getWakeupTick() + duration);
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | SLEEP requested: " << duration << "ms (handled by scheduler)";
    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
