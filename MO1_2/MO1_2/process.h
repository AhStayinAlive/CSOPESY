#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <atomic>
#include <memory>
#include <vector>
#include <map>

struct Instruction {
    std::string opcode;
    std::string arg1, arg2, arg3;
    std::vector<Instruction> subInstructions; 
    int loopCount = 0;                         
};

struct Process {
    int pid;
    std::string name;
    std::string startTime;
    std::string endTime;
    int totalInstructions;
    size_t instructionPointer;
    int coreAssigned;
    bool isRunning;
    bool isFinished;
    bool isDetached = false;
    std::shared_ptr<std::atomic<int>> completedInstructions;
    std::vector<Instruction> instructions;
    std::vector<std::string> logs;

    std::map<std::string, unsigned short> memory;  
};

extern std::vector<std::shared_ptr<Process>> allProcesses;


std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns);

#endif
