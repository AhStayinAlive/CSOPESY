#include "scheduler.h"
#include "utils.h"
#include "config.h"
#include "ProcessManager.h"
#include "instruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "DeclareInstruction.h"
#include "SleepInstruction.h"
#include "PrintInstruction.h"
#include "ForInstruction.h"
#include <sstream>

#include <iomanip>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>

// Global scheduler state
static std::queue<std::shared_ptr<Process>> readyQueue;
static std::mutex schedulerMutex;
static std::condition_variable schedulerCV;
static std::atomic<bool> schedulerRunning{ false };
static std::atomic<bool> shouldStop{ false };
static std::vector<std::thread> cpuWorkers;
static std::vector<std::shared_ptr<std::atomic<bool>>> coreAvailable;

// Scheduler configuration
static SchedulerType currentSchedulerType = SchedulerType::FCFS;
static int timeQuantum = 3;
static std::atomic<int> globalCpuTick{ 0 };

// CPU utilization tracking
static std::mutex utilizationMutex;
static std::chrono::steady_clock::time_point lastUtilizationUpdate;
static double currentUtilization = 0.0;

// Sleep queue for processes with wake-up times
static auto sleepCmp = [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
    return a->getWakeupTick() > b->getWakeupTick();
    };
static std::priority_queue<std::shared_ptr<Process>,
    std::vector<std::shared_ptr<Process>>,
    decltype(sleepCmp)> sleepQueue(sleepCmp);

bool executeSingleInstruction(std::shared_ptr<Process> proc, std::shared_ptr<Instruction> instruction, int coreId) {
    try {
        instruction->execute(proc, coreId);
        (*proc->completedInstructions)++;

        std::string timestamp = getCurrentTimestamp();
        std::ostringstream logEntry;
        logEntry << "[" << timestamp << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | ";

        if (auto add = dynamic_cast<AddInstruction*>(instruction.get())) {
            int lhs = std::stoi(add->arg1);
            int rhs = std::stoi(add->arg2);
            int result = lhs + rhs;
            logEntry << "ADD: " << lhs << " + " << rhs << " = " << result
                << " → " << add->resultVar;
        }
        else if (auto sub = dynamic_cast<SubtractInstruction*>(instruction.get())) {
            int lhs = std::stoi(sub->arg1);
            int rhs = std::stoi(sub->arg2);
            int result = lhs - rhs;
            logEntry << "SUBTRACT: " << lhs << " - " << rhs << " = " << result
                << " → " << sub->resultVar;
        }
        else if (auto loop = dynamic_cast<ForInstruction*>(instruction.get())) {
            logEntry << "FOR Loop ×" << loop->getIterations();
        }
        else if (auto sleep = dynamic_cast<SleepInstruction*>(instruction.get())) {
            logEntry << "SLEEP: " << sleep->getDuration() << " ms";
        }
        else if (auto print = dynamic_cast<PrintInstruction*>(instruction.get())) {
            logEntry << "PRINT: \"" << print->getMessage() << "\"";
        }
        else {
            logEntry << "Executed unknown instruction type";
        }

        std::string finalLog = logEntry.str();
        logToFile(proc->name, finalLog, coreId);
        proc->logs.push_back(finalLog);

        return true;

    }
    catch (const std::exception& e) {
        logToFile(proc->name, "Error executing instruction: " + std::string(e.what()), coreId);
        return false;
    }
}

void executeInstructions(std::shared_ptr<Process>& proc, int coreId, int delayMs) {
    int quantumRemaining = timeQuantum;
    bool shouldPreempt = false;

    while (proc->instructionPointer < static_cast<int>(proc->instructions.size()) && !shouldPreempt) {
        // Check if process should sleep
        if (proc->getWakeupTick() > globalCpuTick.load()) {
            std::lock_guard<std::mutex> lock(schedulerMutex);
            sleepQueue.push(proc);
            proc->isRunning = false;
            return;
        }

        // Execute instruction
        if (!executeSingleInstruction(proc, proc->instructions[proc->instructionPointer], coreId)) {
            proc->logs.push_back("Execution failed at instruction " + std::to_string(proc->instructionPointer + 1));
            break;
        }

        proc->instructionPointer++;
        globalCpuTick++;

        // Add execution delay
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        // Check for preemption in Round Robin
        if (currentSchedulerType == SchedulerType::ROUND_ROBIN) {
            quantumRemaining--;
            if (quantumRemaining <= 0) {
                shouldPreempt = true;
                // Re-queue the process if not finished
                if (proc->instructionPointer < static_cast<int>(proc->instructions.size())) {
                    std::lock_guard<std::mutex> lock(schedulerMutex);
                    readyQueue.push(proc);
                }
            }
        }
    }

    // Process completed or failed
    if (proc->instructionPointer >= static_cast<int>(proc->instructions.size())) {
        proc->endTime = getCurrentTimestamp();
        proc->isFinished = true;
        logToFile(proc->name, "Process completed successfully", coreId);
    }

    proc->isRunning = false;
    *coreAvailable[coreId] = true;
}

void cpuWorker(int coreId, int delayMs) {
    while (!shouldStop.load()) {
        std::shared_ptr<Process> proc;

        {
            std::unique_lock<std::mutex> lock(schedulerMutex);
            schedulerCV.wait(lock, [&] {
                // Wake up sleeping processes
                while (!sleepQueue.empty() && sleepQueue.top()->getWakeupTick() <= globalCpuTick.load()) {
                    auto wakeProc = sleepQueue.top();
                    sleepQueue.pop();
                    readyQueue.push(wakeProc);
                }

                return !readyQueue.empty() || shouldStop.load();
                });

            if (shouldStop.load()) break;

            if (readyQueue.empty()) continue;

            proc = readyQueue.front();
            readyQueue.pop();
            proc->coreAssigned = coreId;
            proc->isRunning = true;
            *coreAvailable[coreId] = false;
        }

        // Set start time if this is the first execution
        if (proc->startTime.empty()) {
            proc->startTime = getCurrentTimestamp();
        }

        // Execute the process
        executeInstructions(proc, coreId, delayMs);
    }
}

void updateCpuUtilization() {
    auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(utilizationMutex);
        int runningProcesses = ProcessManager::getRunningProcessCount();
        int totalCores = Config::getInstance().numCPU;

        if (totalCores > 0) {
            currentUtilization = (static_cast<double>(runningProcesses) / totalCores) * 100.0;
        }
        else {
            currentUtilization = 0.0;
        }

        lastUtilizationUpdate = now;
    }
}

void startScheduler(const Config& config) {
    if (schedulerRunning.load()) {
        std::cout << "Scheduler is already running.\n";
        return;
    }

    shouldStop = false;
    schedulerRunning = true;

    // Set scheduler type
    if (config.scheduler == "RR") {
        currentSchedulerType = SchedulerType::ROUND_ROBIN;
        timeQuantum = config.quantumCycles;
    }
    else {
        currentSchedulerType = SchedulerType::FCFS;
    }

    // Initialize core availability tracking
    coreAvailable.clear();
    coreAvailable.resize(config.numCPU);
    coreAvailable.resize(config.numCPU);
    for (int i = 0; i < config.numCPU; ++i) {
        coreAvailable[i] = std::make_shared<std::atomic<bool>>(true);
    }

    // Start CPU worker threads
    cpuWorkers.clear();
    for (int i = 0; i < config.numCPU; ++i) {
        cpuWorkers.emplace_back(cpuWorker, i, config.delayPerInstruction);
    }

    // Start utilization tracking thread
    std::thread utilizationThread([]() {
        while (schedulerRunning.load()) {
            updateCpuUtilization();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        });
    utilizationThread.detach();

    std::cout << "Scheduler started with " << config.numCPU << " cores using "
        << (currentSchedulerType == SchedulerType::FCFS ? "FCFS" : "Round Robin")
        << " scheduling.\n";
}

void stopScheduler() {
    if (!schedulerRunning.load()) {
        return;
    }

    shouldStop = true;
    schedulerRunning = false;

    // Notify all waiting threads
    schedulerCV.notify_all();

    // Wait for all worker threads to finish
    for (auto& worker : cpuWorkers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    cpuWorkers.clear();
    coreAvailable.clear();

    std::cout << "Scheduler stopped.\n";
}

void addProcess(std::shared_ptr<Process> proc) {
    if (!proc) return;

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        readyQueue.push(proc);
    }
    schedulerCV.notify_one();
}

void generateReport() {
    std::ofstream file("csopesy-log.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not create report file.\n";
        return;
    }

    auto allProcesses = ProcessManager::getAllProcesses();
    auto runningProcesses = ProcessManager::getRunningProcesses();
    auto finishedProcesses = ProcessManager::getFinishedProcesses();
    auto waitingProcesses = ProcessManager::getWaitingProcesses();

    file << "CSOPESY CPU Utilization Report\n";
    file << "Generated: " << getCurrentTimestamp() << "\n\n";

    // System configuration
    file << "System Configuration:\n";
    file << "  CPU Cores: " << Config::getInstance().numCPU << "\n";
    file << "  Scheduler: " << Config::getInstance().scheduler << "\n";
    if (currentSchedulerType == SchedulerType::ROUND_ROBIN) {
        file << "  Time Quantum: " << timeQuantum << " cycles\n";
    }
    file << "  Delay per Instruction: " << Config::getInstance().delayPerInstruction << "ms\n\n";

    // Utilization stats
    double utilization = ProcessManager::getCpuUtilization();
    file << "CPU Utilization Statistics:\n";
    file << "  Current Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
    file << "  Total Processes: " << allProcesses.size() << "\n";
    file << "  Running Processes: " << runningProcesses.size() << "\n";
    file << "  Waiting Processes: " << waitingProcesses.size() << "\n";
    file << "  Finished Processes: " << finishedProcesses.size() << "\n\n";

    // Core usage
    file << "Core Usage:\n";
    for (int i = 0; i < Config::getInstance().numCPU; ++i) {
        std::string status = (*coreAvailable[i]) ? "Available" : "Busy";
        file << "  Core " << i << ": " << status;

        // Find the process assigned to this core
        for (const auto& proc : runningProcesses) {
            if (proc->coreAssigned == i) {
                file << " | Running: " << proc->name << " (PID: " << proc->pid << ")";
                break;
            }
        }
        file << "\n";
    }

    file << "\nFinished Processes:\n";
    if (finishedProcesses.empty()) {
        file << "  [None]\n";
    }
    else {
        for (const auto& proc : finishedProcesses) {
            file << "  " << proc->name << " (PID: " << proc->pid << ") - Completed at " << proc->endTime << "\n";
        }
    }

    file.close();
    std::cout << "Report generated: csopesy-log.txt\n";
}
