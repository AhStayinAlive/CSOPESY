#include "scheduler.h"
#include "utils.h"
#include "config.h"
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <chrono>
#include <fstream>
#include "instruction_executor.h"

static std::queue<std::shared_ptr<Process>> readyQueue;
static std::mutex mtx;
static std::condition_variable cv;
bool stop = false;
bool running = false;
static Config globalConfig;

void executeInstructions(std::shared_ptr<Process>& proc, const std::vector<Instruction>& instructions, int coreId, int delay) {
    for (size_t i = 0; i < instructions.size(); ++i) {
        proc->instructionPointer = i;  // track current instruction index
        if (!executeSingleInstruction(proc, instructions[i], coreId)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

}

void cpuWorker(int coreId, int delay) {
    while (true) {
        std::shared_ptr<Process> proc;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return !readyQueue.empty() || stop; });
            if (readyQueue.empty() && stop) break;
            proc = readyQueue.front(); readyQueue.pop();
            proc->coreAssigned = coreId;
            proc->isRunning = true;
        }

        proc->startTime = getCurrentTimestamp();

        executeInstructions(proc, proc->instructions, coreId, delay);

        proc->endTime = getCurrentTimestamp();
        proc->isRunning = false;
        proc->isFinished = true;
    }
}

void startScheduler(const Config& config) {
    globalConfig = config;
    running = true;
    stop = false;

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
