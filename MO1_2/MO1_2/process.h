// process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>

class Instruction;

struct Process {
    int pid;
    std::string name;
    int instructionPointer;
    int coreAssigned;
    bool isRunning;
    bool isFinished;
    bool isDetached;
    std::string startTime;
    std::string endTime;
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::unordered_map<std::string, uint16_t> memory;
    std::vector<std::string> logs;
    std::shared_ptr<std::atomic<int>> completedInstructions;
    int totalInstructions;
    int wakeupTick = 0;

    int getWakeupTick() const { return wakeupTick; }
    void setWakeupTick(int tick) { wakeupTick = tick; }
};

// Function declaration
std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns);

#endif // PROCESS_H