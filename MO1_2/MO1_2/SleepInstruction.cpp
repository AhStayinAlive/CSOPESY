// SleepInstruction.cpp
#include "SleepInstruction.h"
#include "process.h"
#include "utils.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstdint>

SleepInstruction::SleepInstruction(int ms) : duration(ms) {}

void SleepInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Simple implementation - just sleep for the specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | SLEEP: for " + std::to_string(duration) + "ms";

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), proc->coreAssigned);
}