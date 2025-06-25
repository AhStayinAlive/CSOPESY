#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>

struct Instruction {
    std::string opcode;
    std::string arg1, arg2, arg3;
};

struct Process {
    int pid;
    std::string name;
    std::string startTime;
    std::string endTime;
    int totalInstructions;
    int instructionPointer;
    int coreAssigned;
    bool isRunning;
    bool isFinished;
    bool isDetached = false;
    std::shared_ptr<std::atomic<int>> completedInstructions;
    std::vector<Instruction> instructions;
    std::vector<std::string> logs;
    std::unordered_map<std::string, uint16_t> variables;
};

std::shared_ptr<Process> generateRandomProcess(int pid, int minIns, int maxIns);

#endif