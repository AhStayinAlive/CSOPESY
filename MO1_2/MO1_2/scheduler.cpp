#include "scheduler.h"
#include "MemoryManager.h"               // ✅ Updated from FlatMemoryAllocator
#include "ProcessManager.h"
#include "config.h"
#include "utils.h"
#include "instruction.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>

// Global variables
std::vector<std::shared_ptr<Process>> allProcesses;
bool stop = false;
bool running = false;
SchedulerType schedulerType = SchedulerType::ROUND_ROBIN;
int cpuTick = 0;
int timeQuantum = 3;

// Global scheduler instance
static ProcessScheduler globalScheduler;

ProcessScheduler::~ProcessScheduler() {
    stop();
}

void ProcessScheduler::start(const Config& config) {
    if (running.load()) {
        std::cout << "Scheduler is already running.\n";
        return;
    }

    numCPU = config.numCPU;
    timeQuantum = config.quantumCycles;
    delayPerInstruction = config.delayPerInstruction;

    std::string sched = config.scheduler;
    std::transform(sched.begin(), sched.end(), sched.begin(), ::toupper);

    schedulerType = (sched == "RR" || sched == "ROUND_ROBIN")
        ? SchedulerType::ROUND_ROBIN
        : SchedulerType::FCFS;

    coreAvailable.assign(numCPU, true);
    shouldStop = false;
    running = true;
    quantumCycle = 0;

    workerThreads.clear();
    for (int i = 0; i < numCPU; ++i) {
        workerThreads.emplace_back(&ProcessScheduler::cpuWorker, this, i);
    }
    workerThreads.emplace_back(&ProcessScheduler::schedulerLoop, this);

    std::cout << "ProcessScheduler started with " << numCPU << " cores using "
        << (schedulerType == SchedulerType::FCFS ? "FCFS" : "Round Robin")
        << " scheduling.\n";
}

void ProcessScheduler::stop() {
    if (!running.load()) return;

    shouldStop = true;
    running = false;
    queueCV.notify_all();

    for (auto& thread : workerThreads) {
        if (thread.joinable()) thread.join();
    }

    workerThreads.clear();
    coreAvailable.clear();
    std::cout << "ProcessScheduler stopped.\n";
}

void ProcessScheduler::addProcess(std::shared_ptr<Process> process) {
    if (!process) return;

    process->setStatus(ProcessStatus::READY);
    process->arrivalTime = getCurrentTimestamp();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(process);
    }

    queueCV.notify_one();
    // Comment out this line to reduce output spam
    // std::cout << "Process " << process->name << " (PID: " << process->pid << ") added to ready queue\n";
}

void ProcessScheduler::schedulerLoop() {
    while (!shouldStop.load()) {
        quantumCycle.fetch_add(1);
        MemoryManager::getInstance().incrementCycle(); // ✅ updated from FlatMemoryAllocator
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ProcessScheduler::cpuWorker(int coreId) {
    while (!shouldStop.load()) {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] {
                return !readyQueue.empty() || shouldStop.load();
                });

            if (shouldStop.load()) break;
            if (readyQueue.empty()) continue;

            process = readyQueue.front();
            readyQueue.pop();
        }

        if (!tryAllocateMemory(process)) {
            std::lock_guard<std::mutex> lock(queueMutex);
            readyQueue.push(process);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        coreAvailable[coreId] = false;
        process->coreAssigned = coreId;
        process->setStatus(ProcessStatus::RUNNING);

        executeProcess(process, coreId);
        coreAvailable[coreId] = true;
    }
}

bool ProcessScheduler::tryAllocateMemory(std::shared_ptr<Process> process) {
    if (process->getBaseAddress() != -1) return true;
    return MemoryManager::getInstance().allocate(process); // ✅ updated
}

void ProcessScheduler::executeProcess(std::shared_ptr<Process> process, int coreId) {
    if (process->startTime.empty()) {
        process->startTime = getCurrentTimestamp();
    }

    int quantumRemaining = timeQuantum;
    bool shouldPreempt = false;
    process->log("Process started execution on core " + std::to_string(coreId));

    while (process->instructionPointer < static_cast<int>(process->instructions.size()) &&
        !shouldPreempt && !shouldStop.load()) {

        if (process->getWakeupTick() > quantumCycle.load()) {
            process->setStatus(ProcessStatus::READY);
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                readyQueue.push(process);
            }
            process->log("Process went to sleep");
            return;
        }

        try {
            auto instruction = process->instructions[process->instructionPointer];
            instruction->execute(process, coreId);
            (*process->completedInstructions)++;
            process->instructionPointer++;

            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerInstruction));

            if (schedulerType == SchedulerType::ROUND_ROBIN && --quantumRemaining <= 0) {
                shouldPreempt = true;
                if (process->instructionPointer < static_cast<int>(process->instructions.size())) {
                    process->setStatus(ProcessStatus::READY);
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(process);
                    }
                    process->log("Process preempted (quantum expired)");
                }
            }
        }
        catch (const std::exception& e) {
            process->log("Error executing instruction: " + std::string(e.what()));
            break;
        }
    }

    if (process->instructionPointer >= static_cast<int>(process->instructions.size())) {
        process->setStatus(ProcessStatus::DONE);
        process->endTime = getCurrentTimestamp();
        process->log("Process completed successfully");
        deallocateProcessMemory(process);
        process->writeLogToFile();
        std::cout << "Process " << process->name << " (PID: " << process->pid
            << ") completed on core " << coreId << "\n";
    }
}

void ProcessScheduler::deallocateProcessMemory(std::shared_ptr<Process> process) {
    if (process->getBaseAddress() != -1) {
        MemoryManager::getInstance().deallocate(process); // ✅ updated
    }
}

size_t ProcessScheduler::getReadyQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return readyQueue.size();
}

void ProcessScheduler::generateReport() {
    std::ofstream file("csopesy-log.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not create report file.\n";
        return;
    }

    auto all = ProcessManager::getAllProcesses();
    auto running = ProcessManager::getRunningProcesses();
    auto finished = ProcessManager::getFinishedProcesses();

    file << "CSOPESY CPU and Memory Utilization Report\n";
    file << "Generated: " << getCurrentTimestamp() << "\n\n";

    file << "System Configuration:\n";
    file << "  CPU Cores: " << numCPU << "\n";
    file << "  Scheduler: " << (schedulerType == SchedulerType::FCFS ? "FCFS" : "Round Robin") << "\n";
    if (schedulerType == SchedulerType::ROUND_ROBIN) {
        file << "  Time Quantum: " << timeQuantum << "\n";
    }
    file << "  Delay per Instruction: " << delayPerInstruction << "ms\n\n";

    auto& mem = MemoryManager::getInstance(); // ✅ updated
    file << "Memory Information:\n";
    file << "  Total Memory: " << mem.getTotalMemory() << " bytes\n";
    file << "  Used Memory: " << mem.getUsedMemory() << " bytes\n";
    file << "  Free Memory: " << mem.getFreeMemory() << " bytes\n\n";

    file << "Process Statistics:\n";
    file << "  Total Processes: " << all.size() << "\n";
    file << "  Running Processes: " << running.size() << "\n";
    file << "  Finished Processes: " << finished.size() << "\n";
    file << "  Ready Queue Size: " << getReadyQueueSize() << "\n\n";

    file << "Running Processes:\n";
    if (running.empty()) file << "  None\n";
    else {
        for (const auto& p : running) {
            file << "  " << p->name << " (PID: " << p->pid
                << ", Core: " << p->coreAssigned.load()
                << ", Progress: " << std::fixed << std::setprecision(2)
                << p->getProgress() << "%)\n";
        }
    }
    file << "\nFinished Processes:\n";
    if (finished.empty()) file << "  None\n";
    else {
        for (const auto& p : finished) {
            file << "  " << p->name << " (PID: " << p->pid
                << ", Start: " << p->startTime
                << ", End: " << p->endTime << ")\n";
        }
    }

    file.close();
    std::cout << "Report generated: csopesy-log.txt\n";
}

void ProcessScheduler::printStatus() const {
    std::cout << "\n=== Scheduler Status ===\n";
    std::cout << "Running: " << (running.load() ? "Yes" : "No") << "\n";
    std::cout << "Current Cycle: " << quantumCycle.load() << "\n";
    std::cout << "Ready Queue Size: " << getReadyQueueSize() << "\n";
    std::cout << "Scheduler Type: " << (schedulerType == SchedulerType::FCFS ? "FCFS" : "Round Robin") << "\n";
    std::cout << "CPU Cores: " << numCPU << "\n";
    if (schedulerType == SchedulerType::ROUND_ROBIN)
        std::cout << "Time Quantum: " << timeQuantum << "\n";
    std::cout << "========================\n\n";
}
void startScheduler(const Config& config) {
    globalScheduler.start(config);
}

void stopScheduler() {
    globalScheduler.stop();
}

void addProcess(std::shared_ptr<Process> p) {
    globalScheduler.addProcess(p);
}

void generateReport() {
    globalScheduler.generateReport();
}