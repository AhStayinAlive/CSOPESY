// SleepInstruction.cpp
#include "SleepInstruction.h"
#include "utils.h"
#include <thread>
#include <chrono>
#include "process.h"
SleepInstruction::SleepInstruction(int ms) : duration(ms) {}

void SleepInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Simple implementation - just sleep for the specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));

    logToFile(proc->name, "SLEEP for " + std::to_string(duration) + "ms", coreId);
}