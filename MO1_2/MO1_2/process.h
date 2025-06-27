// process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

class Instruction;

struct Process {
    // Identity and control
    int pid;
    std::string name;
    int instructionPointer = 0;
    int coreAssigned = -1;
    bool isRunning = false;
    bool isFinished = false;
    bool isDetached = false;

    // Timing
    std::string startTime;
    std::string endTime;

    // Instructions and execution tracking
    std::vector<std::shared_ptr<Instruction>> instructions;
    int totalInstructions = 0;
    std::shared_ptr<std::atomic<int>> completedInstructions = std::make_shared<std::atomic<int>>(0);

    // Process memory (vars) and logs
    std::unordered_map<std::string, uint16_t> memory; // variable map (was: variables)
    std::vector<std::string> logs;

    // Sleep control
    int wakeupTick = 0;

    // Getters and setters
    int getWakeupTick() const { return wakeupTick; }
    void setWakeupTick(int tick) { wakeupTick = tick; }
};

// Function to generate a random process
std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns);

#endif // PROCESS_H
