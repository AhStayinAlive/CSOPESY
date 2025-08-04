// process.h
#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

class Instruction;

struct PageTableEntry {
    int frameNumber = -1; // Physical frame number
    bool valid = false;   // Is page in memory
    bool dirty = false;   // Was page modified
    int lastUsedTick = 0; // For LRU (optional for FIFO)
};

struct Process {
    // Identity and control
    int pid = -1;
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

    // Virtual memory and page table
    std::unordered_map<int, PageTableEntry> pageTable;

    // Legacy memory (can be deprecated or used as cache)
    std::unordered_map<std::string, uint16_t> memory;
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