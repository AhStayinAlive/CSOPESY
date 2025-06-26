#include "scheduler.h"
#include "utils.h"
#include "config.h"
#include "instruction_executor.h"
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <chrono>
#include <fstream>
#include <functional>

static std::queue<std::shared_ptr<Process>> readyQueue;
static std::mutex mtx;
static std::condition_variable cv;
bool stop = false;
bool running = false;
SchedulerType schedulerType = SchedulerType::FCFS;
int cpuTick = 0;
int timeQuantum = 3;

// Add the missing global variable
std::vector<std::shared_ptr<Process>> allProcesses;

// Sleep queue for delayed requeueing
static auto cmp = [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
    return a->getWakeupTick() > b->getWakeupTick();
    };
static std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, decltype(cmp)> sleepQueue(cmp);

void executeInstructions(std::shared_ptr<Process>& proc, int coreId, int delay) {
    int quantumRemaining = timeQuantum;

    while (proc->instructionPointer < static_cast<int>(proc->instructions.size())) {
        // Check for wake-up tick skip
        if (proc->getWakeupTick() > cpuTick) {
            std::lock_guard<std::mutex> lock(mtx);
            sleepQueue.push(proc);
            return;
        }

        if (!executeSingleInstruction(proc, proc->instructions[proc->instructionPointer], coreId)) break;
        proc->instructionPointer++;
        cpuTick++;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        if (schedulerType == SchedulerType::ROUND_ROBIN) {
            quantumRemaining--;
            if (quantumRemaining == 0) {
                std::lock_guard<std::mutex> lock(mtx);
                readyQueue.push(proc);
                return; // Preempt and yield
            }
        }
    }

    proc->endTime = getCurrentTimestamp();
    proc->isRunning = false;
    proc->isFinished = true;
}

void cpuWorker(int coreId, int delay) {
    while (true) {
        std::shared_ptr<Process> proc;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] {
                return !readyQueue.empty() || !sleepQueue.empty() || stop;
                });

            if (stop) break; // Add this check to properly exit

            // Wake up sleeping processes
            while (!sleepQueue.empty() && sleepQueue.top()->getWakeupTick() <= cpuTick) {
                readyQueue.push(sleepQueue.top());
                sleepQueue.pop();
            }

            if (readyQueue.empty()) continue;

            proc = readyQueue.front(); readyQueue.pop();
            proc->coreAssigned = coreId;
            proc->isRunning = true;
        }

        proc->startTime = getCurrentTimestamp();
        executeInstructions(proc, coreId, delay);
    }
}

void startScheduler(const Config& config) {
    // Use the passed config parameter directly instead of copying
    running = true;
    stop = false;

    // Set scheduler type based on config
    if (config.scheduler == "RR") {
        schedulerType = SchedulerType::ROUND_ROBIN;
        timeQuantum = config.quantumCycles;
    }
    else {
        schedulerType = SchedulerType::FCFS;
    }

    for (int i = 0; i < config.numCPU; ++i)
        std::thread(cpuWorker, i, config.delayPerInstruction * 100).detach();
}

void stopScheduler() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();
    running = false;
}

void addProcess(std::shared_ptr<Process> p) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        allProcesses.push_back(p);
        readyQueue.push(p);
    }
    cv.notify_one();
}

void generateReport() {
    std::ofstream file("csopesy-log.txt");

    file << "CPU Utilization Report\n\nRunning Processes:\n";
    for (auto& proc : allProcesses) {
        if (proc->isRunning && !proc->isFinished)
            file << proc->name << " started at " << proc->startTime << ", on Core " << proc->coreAssigned << "\n";
    }

    file << "\nFinished Processes:\n";
    for (auto& proc : allProcesses) {
        if (proc->isFinished)
            file << proc->name << " ended at " << proc->endTime << "\n";
    }

    file.close();
    std::cout << "Utilization report written to csopesy-log.txt\n";
}