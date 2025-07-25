#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "config.h"
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

enum class SchedulerType {
    FCFS,
    ROUND_ROBIN
};

class ProcessScheduler {
private:
    std::queue<std::shared_ptr<Process>> readyQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCV;

    std::atomic<bool> running{ false };
    std::atomic<bool> shouldStop{ false };
    std::atomic<int> quantumCycle{ 0 };

    std::vector<std::thread> workerThreads;
    std::vector<bool> coreAvailable;

    SchedulerType schedulerType = SchedulerType::ROUND_ROBIN;
    int timeQuantum = 3;
    int delayPerInstruction = 100;
    int numCPU = 4;
    bool gracefulStop = false;

    void schedulerLoop();
    void cpuWorker(int coreId);
    bool tryAllocateMemory(std::shared_ptr<Process> process);
    void executeProcess(std::shared_ptr<Process> process, int coreId);
    void deallocateProcessMemory(std::shared_ptr<Process> process);

public:
    ProcessScheduler() = default;
    ~ProcessScheduler();

    void start(const Config& config);
    void stop();
    void addProcess(std::shared_ptr<Process> process);

    bool isRunning() const { return running.load(); }
    int getCurrentCycle() const { return quantumCycle.load(); }
    size_t getReadyQueueSize() const;

    void generateReport();
    void printStatus() const;
};

// Global variables used externally
extern std::vector<std::shared_ptr<Process>> allProcesses;
extern bool stop;
extern bool running;
extern SchedulerType schedulerType;
extern int cpuTick;
extern int timeQuantum;

// Global helper functions (exposed to CLIManager or main.cpp)
void startScheduler(const Config& config);
void stopScheduler();
void addProcess(std::shared_ptr<Process> p);
void generateReport();

#endif // SCHEDULER_H
