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

std::vector<std::shared_ptr<Process>> allProcesses;
static std::queue<std::shared_ptr<Process>> readyQueue;
static std::mutex mtx;
static std::condition_variable cv;
bool stop = false;
bool running = false;
static Config globalConfig;

void executeInstructions(std::shared_ptr<Process>& proc, const std::vector<Instruction>& instructions, int coreId, int delay) {
    for (const auto& ins : instructions) {
        if (ins.opcode == "PRINT") {
            std::string msg = ins.arg1;
            if (proc->variables.count(ins.arg1)) {
                msg += " = " + std::to_string(proc->variables[ins.arg1]);
            }
            logToFile(proc->name, msg, coreId);
            proc->logs.push_back(msg);
        }
        else if (ins.opcode == "SLEEP") {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * std::stoi(ins.arg1)));
        }
        else if (ins.opcode == "ADD") {
            uint16_t lhs = proc->variables.count(ins.arg2) ? proc->variables[ins.arg2] : std::stoi(ins.arg2);
            uint16_t rhs = proc->variables.count(ins.arg3) ? proc->variables[ins.arg3] : std::stoi(ins.arg3);
            proc->variables[ins.arg1] = lhs + rhs;
            logToFile(proc->name, "ADD " + ins.arg1 + " = " + std::to_string(lhs) + " + " + std::to_string(rhs), coreId);
        }
        else if (ins.opcode == "SUBTRACT") {
            uint16_t lhs = proc->variables.count(ins.arg2) ? proc->variables[ins.arg2] : std::stoi(ins.arg2);
            uint16_t rhs = proc->variables.count(ins.arg3) ? proc->variables[ins.arg3] : std::stoi(ins.arg3);
            proc->variables[ins.arg1] = lhs - rhs;
            logToFile(proc->name, "SUBTRACT " + ins.arg1 + " = " + std::to_string(lhs) + " - " + std::to_string(rhs), coreId);
        }
        else if (ins.opcode == "DECLARE") {
            uint16_t val = std::stoi(ins.arg2);
            proc->variables[ins.arg1] = val;
            logToFile(proc->name, "DECLARE " + ins.arg1 + " = " + ins.arg2, coreId);
        }
        else if (ins.opcode == "FOR") {
            int repeat = std::stoi(ins.arg2);
            for (int i = 0; i < repeat; ++i) {
                executeInstructions(proc, ins.subInstructions, coreId, delay);
            }
        }

        (*proc->completedInstructions)++;
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
