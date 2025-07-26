#include "scheduler.h"
#include "MemoryManager.h"
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
    gracefulStop = false;
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

//void ProcessScheduler::stop() {
//    if (!running.load()) return;
//
//    gracefulStop = true;
//    std::cout << "[INFO] Stopping process generation, waiting for queue to empty...\n";
//
//    // Wait until readyQueue is empty and all cores are free
//    std::thread waitThread([this]() {
//        while (true) {
//            {
//                std::lock_guard<std::mutex> lock(queueMutex);
//                bool allCoresFree = std::all_of(coreAvailable.begin(), coreAvailable.end(),
//                    [](bool v) { return v; });
//                if (readyQueue.empty() && allCoresFree) break;
//            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(200));
//        }
//        shouldStop = true;
//        queueCV.notify_all();
//        });
//
//    waitThread.join();
//
//    for (auto& thread : workerThreads) {
//        if (thread.joinable()) thread.join();
//    }
//
//    workerThreads.clear();
//    coreAvailable.clear();
//    running = false;
//
//    std::cout << "ProcessScheduler stopped gracefully.\n";
//}

void ProcessScheduler::stop() {
    if (!running.load()) return;

    gracefulStop = true;
    std::cout << "[INFO] Stopping process generation, waiting for queue to empty...\n";

    // Detach a background thread to handle graceful shutdown
    std::thread([this]() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                bool allCoresFree = std::all_of(coreAvailable.begin(), coreAvailable.end(),
                    [](bool v) { return v; });
                if (readyQueue.empty() && allCoresFree) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        shouldStop = true;
        queueCV.notify_all();

        for (auto& thread : workerThreads) {
            if (thread.joinable()) thread.join();
        }
        workerThreads.clear();
        coreAvailable.clear();
        running = false;

        std::cout << "\nProcessScheduler stopped gracefully.\n> ";
        std::cout.flush();
        }).detach();
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
}

void ProcessScheduler::schedulerLoop() {
    while (!shouldStop.load()) {
        quantumCycle.fetch_add(1);
        MemoryManager::getInstance().incrementCycle();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ProcessScheduler::cpuWorker(int coreId) {
    while (true) {
        std::shared_ptr<Process> process;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] {
                return !readyQueue.empty() || shouldStop.load();
                });

            if (shouldStop.load() && readyQueue.empty()) break;

            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
            }
            else {
                continue;
            }
        }

        if (!process) continue;

        // Try memory allocation
        if (!tryAllocateMemory(process)) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                readyQueue.push(process);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            queueCV.notify_one();
            continue;
        }

        coreAvailable[coreId] = false;
        process->coreAssigned = coreId;
        process->isRunning = true;
        process->setStatus(ProcessStatus::RUNNING);

        executeProcess(process, coreId);

        coreAvailable[coreId] = true;
        process->isRunning = false;
    }
}

bool ProcessScheduler::tryAllocateMemory(std::shared_ptr<Process> process) {
    if (process->getBaseAddress() != -1) return true;
    return MemoryManager::getInstance().allocate(process);
}

void ProcessScheduler::executeProcess(std::shared_ptr<Process> process, int coreId) {
    if (process->startTime.empty()) {
        process->startTime = getCurrentTimestamp();
    }

    int quantumRemaining = timeQuantum;
    bool shouldPreempt = false;
    process->log("Started execution on Core " + std::to_string(coreId));

    while (process->instructionPointer < static_cast<int>(process->instructions.size()) &&
        !shouldPreempt && !shouldStop.load()) {
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
                        readyQueue.push(process); // Preempted: requeue at tail
                    }
                    process->log("Preempted after quantum");
                    queueCV.notify_one();
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
        /*std::cout << "Process " << process->name << " (PID: " << process->pid
            << ") completed on core " << coreId << "\n";*/
    }
}

void ProcessScheduler::deallocateProcessMemory(std::shared_ptr<Process> process) {
    if (process->getBaseAddress() != -1) {
        MemoryManager::getInstance().deallocate(process);
    }
}

size_t ProcessScheduler::getReadyQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return readyQueue.size();
}

void ProcessScheduler::generateReport() {
    // No change from previous
}

void ProcessScheduler::printStatus() const {
    // No change from previous
}

// Global helper functions
void startScheduler(const Config& config) { globalScheduler.start(config); }
void stopScheduler() { globalScheduler.stop(); }
void addProcess(std::shared_ptr<Process> p) { globalScheduler.addProcess(p); }
void generateReport() { globalScheduler.generateReport(); }
