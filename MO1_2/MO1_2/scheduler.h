
// scheduler.h
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <memory>
#include <vector>

class Config;

enum class SchedulerType {
    FCFS,
    ROUND_ROBIN
};

// Global variables
extern std::vector<std::shared_ptr<Process>> allProcesses;
extern bool stop;
extern bool running;
extern SchedulerType schedulerType;
extern int cpuTick;
extern int timeQuantum;

// Function declarations
void startScheduler(const Config& config);
void stopScheduler();
void addProcess(std::shared_ptr<Process> p);
void generateReport();
void executeInstructions(std::shared_ptr<Process>& proc, int coreId, int delay);
void cpuWorker(int coreId, int delay);

#endif // SCHEDULER_H

// ========================================

// instruction_executor.h
#ifndef INSTRUCTION_EXECUTOR_H
#define INSTRUCTION_EXECUTOR_H

#include "process.h"
#include <memory>

class Instruction;

bool executeSingleInstruction(std::shared_ptr<Process> proc,
    std::shared_ptr<Instruction> instruction,
    int coreId);

#endif // INSTRUCTION_EXECUTOR_H