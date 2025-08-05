#include "ProcessManager.h"
#include "DeclareInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "PrintInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include "UserAddInstruction.h"
#include "UserPrintInstruction.h"
#include <random>
#include <sstream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include "config.h"
#include "ReadInstruction.h"
#include "WriteInstruction.h"
#include <iostream>

static std::vector<std::shared_ptr<Process>> allProcesses;
static std::unordered_map<std::string, std::shared_ptr<Process>> processMap;
static int pidCounter = 1000;
static int uniqueProcessCounter = 1;

std::shared_ptr<Process> ProcessManager::createProcess(const std::string& name, int pid, int minInstructions, int maxInstructions, int memoryLimit) {
    auto proc = std::make_shared<Process>();
    proc->pid = pid;
    proc->name = name;
    proc->instructionPointer = 0;
    proc->coreAssigned = -1;
    proc->isRunning = false;
    proc->isFinished = false;
    proc->isDetached = false;
    proc->completedInstructions = std::make_shared<std::atomic<int>>(0);
    proc->virtualMemoryLimit = memoryLimit;

    {
        int spacing = Config::getInstance().memPerFrame / 4;  // 16B if frame = 64B
        int preVarCount = proc->virtualMemoryLimit / spacing;
        for (int i = 0; i < preVarCount; ++i) {
            std::string varName = "prealloc" + std::to_string(i);
            int addr = proc->nextFreeAddress;
            proc->variableTable[varName] = addr;
            proc->nextFreeAddress += spacing;
            MemoryManager::getInstance().write(proc, addr, static_cast<uint8_t>(i + 1));
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instructionCount(minInstructions, maxInstructions);
    std::uniform_int_distribution<> opcodePicker(0, 5);
    std::uniform_int_distribution<> valueDist(1, 100);
    std::uniform_int_distribution<> sleepTime(100, 500);
    std::uniform_int_distribution<> loopCountDist(2, 4);


    try {
        int numInstructions = instructionCount(gen);
        
        proc->totalInstructions = numInstructions;

        int count = 0;

        while (proc->instructions.size() < numInstructions) {
            int op = opcodePicker(gen);
            switch (op) {
            case 0: {
                std::string varName = "var" + std::to_string(count);
                int value = valueDist(gen);
                proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));
                count++;
                break;
            }
            case 1: {
                std::string result = "sum" + std::to_string(count);
                std::string lhs = "var" + std::to_string(count % 5);
                std::string rhs = "var" + std::to_string((count + 1) % 5);
                proc->instructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs));
                count++;
                break;
            }
            case 2: {
                std::string result = "diff" + std::to_string(count);
                std::string lhs = "var" + std::to_string(count % 5);
                std::string rhs = "var" + std::to_string((count + 1) % 5);
                proc->instructions.push_back(std::make_shared<SubtractInstruction>(result, lhs, rhs));
                count++;
                break;
            }
            case 3: {
                std::string message = "Hello from " + name ;
                proc->instructions.push_back(std::make_shared<PrintInstruction>(message));
                count++;
                break;
            }
            case 4: {
                int duration = sleepTime(gen);
                proc->instructions.push_back(std::make_shared<SleepInstruction>(duration));
                count++;
                break;
            }
            case 5: {
                int loopCount = loopCountDist(gen);

                for (int i = 0; i < loopCount && proc->instructions.size() < maxInstructions; ++i) {
                    for (int j = 0; j < 2 && proc->instructions.size() < maxInstructions; ++j) {
                        for (int k = 0; k < 1 && proc->instructions.size() < maxInstructions; ++k) {
                            int subOp = opcodePicker(gen);  // pick like at top level
                            std::string loopMsg =
                                "FOR (outer " + std::to_string(i + 1) +
                                ", mid " + std::to_string(j + 1) +
                                ", inner " + std::to_string(k + 1) + ") ";
                            switch (subOp) {
                            case 0: {
                                std::string varName = "var" + std::to_string(count);
                                int value = valueDist(gen);
                                proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value, loopMsg));
                                count++;
                                break;
                            }
                            case 1: {
                                std::string result = "sum" + std::to_string(count);
                                std::string lhs = "var" + std::to_string(count % 5);
                                std::string rhs = "var" + std::to_string((count + 1) % 5);
                                proc->instructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs, loopMsg));
                                count++;
                                break;
                            }
                            case 2: {
                                std::string result = "diff" + std::to_string(count);
                                std::string lhs = "var" + std::to_string(count % 5);
                                std::string rhs = "var" + std::to_string((count + 1) % 5);
                                proc->instructions.push_back(std::make_shared<SubtractInstruction>(result, lhs, rhs, loopMsg));
                                count++;
                                break;
                            }
                            case 3: {
                                std::string message = "Hello from " + name ;
                                proc->instructions.push_back(std::make_shared<PrintInstruction>(message, loopMsg));
                                count++;
                                break;
                            }
                            case 4: {
                                int duration = sleepTime(gen);
                                proc->instructions.push_back(std::make_shared<SleepInstruction>(duration, loopMsg));
                                count++;
                                break;
                            }
                            }
                        }
                    }
                    
                }
                break;
            }
            }
            
        }
    }
    catch (const std::exception& e) {
        proc->logs.push_back("Process generation failed: " + std::string(e.what()));
        proc->totalInstructions = 0;
    }

    return proc;
}

std::shared_ptr<Process> ProcessManager::createUniqueNamedProcess(int minIns, int maxIns) {
    std::string processName;
    do {
        processName = "process_" + std::to_string(uniqueProcessCounter++);
    } while (processMap.find(processName) != processMap.end());

    const auto& config = Config::getInstance();
    return createProcess(processName, pidCounter++, minIns, maxIns, config.maxMemPerProc);
}

std::shared_ptr<Process> ProcessManager::createNamedProcess(const std::string& name, int memoryLimit) {
    if (processMap.find(name) != processMap.end()) {
        return processMap[name];
    }
    const auto& config = Config::getInstance();
    return createProcess(name, pidCounter++, config.minInstructions, config.maxInstructions, memoryLimit);
}

std::shared_ptr<Process> ProcessManager::findByName(const std::string& name) {
    auto it = processMap.find(name);
    return (it != processMap.end()) ? it->second : nullptr;
}

std::shared_ptr<Process> ProcessManager::findByPid(int pid) {
    auto it = std::find_if(allProcesses.begin(), allProcesses.end(),
        [pid](const std::shared_ptr<Process>& p) { return p->pid == pid; });
    return (it != allProcesses.end()) ? *it : nullptr;
}

void ProcessManager::addProcess(std::shared_ptr<Process> proc) {
    if (!proc) return;
    auto it = std::find_if(allProcesses.begin(), allProcesses.end(),
        [&proc](const std::shared_ptr<Process>& p) { return p->name == proc->name; });
    if (it == allProcesses.end()) {
        allProcesses.push_back(proc);
        processMap[proc->name] = proc;
    }
}

std::vector<std::shared_ptr<Process>> ProcessManager::getAllProcesses() {
    return allProcesses;
}

std::vector<std::shared_ptr<Process>> ProcessManager::getRunningProcesses() {
    std::vector<std::shared_ptr<Process>> running;
    std::copy_if(allProcesses.begin(), allProcesses.end(), std::back_inserter(running),
        [](const std::shared_ptr<Process>& p) { return p->isRunning && !p->isFinished; });
    return running;
}

std::vector<std::shared_ptr<Process>> ProcessManager::getFinishedProcesses() {
    std::vector<std::shared_ptr<Process>> finished;
    std::copy_if(allProcesses.begin(), allProcesses.end(), std::back_inserter(finished),
        [](const std::shared_ptr<Process>& p) { return p->isFinished; });
    return finished;
}

std::vector<std::shared_ptr<Process>> ProcessManager::getWaitingProcesses() {
    std::vector<std::shared_ptr<Process>> waiting;
    std::copy_if(allProcesses.begin(), allProcesses.end(), std::back_inserter(waiting),
        [](const std::shared_ptr<Process>& p) { return !p->isRunning && !p->isFinished; });
    return waiting;
}

int ProcessManager::getProcessCount() {
    return static_cast<int>(allProcesses.size());
}

int ProcessManager::getRunningProcessCount() {
    return static_cast<int>(std::count_if(allProcesses.begin(), allProcesses.end(),
        [](const std::shared_ptr<Process>& p) { return p->isRunning && !p->isFinished; }));
}

int ProcessManager::getFinishedProcessCount() {
    return static_cast<int>(std::count_if(allProcesses.begin(), allProcesses.end(),
        [](const std::shared_ptr<Process>& p) { return p->isFinished; }));
}

double ProcessManager::getCpuUtilization() {
    int numCPU = Config::getInstance().numCPU;
    if (numCPU == 0) return 0.0;
    int running = getRunningProcessCount();
    return (static_cast<double>(running) / numCPU) * 100.0;
}

void ProcessManager::clearAllProcesses() {
    allProcesses.clear();
    processMap.clear();
    pidCounter = 1000;
    uniqueProcessCounter = 1;
}

std::shared_ptr<Process> ProcessManager::createProcessWithInstructions(const std::string& name, int memoryLimit, const std::vector<std::string>& instructions) {
    auto proc = std::make_shared<Process>();
    proc->pid = pidCounter++;
    proc->name = name;
    proc->instructionPointer = 0;
    proc->coreAssigned = -1;
    proc->isRunning = false;
    proc->isFinished = false;
    proc->isDetached = false;
    proc->completedInstructions = std::make_shared<std::atomic<int>>(0);
    proc->virtualMemoryLimit = memoryLimit;
    proc->totalInstructions = instructions.size();

    // Parse instructions 
    for (const auto& instrStr : instructions) {
        std::string trimmed = instrStr;
        // Remove leading/trailing whitespace
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

        std::stringstream ss(trimmed);
        std::string cmd;
        ss >> cmd;

        // Convert to uppercase 
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "DECLARE") {
            std::string varName;
            int value;
            ss >> varName >> value;
            proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));
        }
        else if (cmd == "ADD") {
            std::string result, arg1, arg2;
            ss >> result >> arg1 >> arg2;
            proc->instructions.push_back(std::make_shared<UserAddInstruction>(result, arg1, arg2));
        }
        else if (cmd == "WRITE") {
            std::string hexAddr, varName;
            ss >> hexAddr >> varName;

            int address;
            try {
                if (hexAddr.substr(0, 2) == "0x" || hexAddr.substr(0, 2) == "0X") {
                    address = std::stoi(hexAddr, nullptr, 16);
                }
                else {
                    try {
                        address = std::stoi(hexAddr, nullptr, 16);
                    }
                    catch (...) {
                        address = std::stoi(hexAddr, nullptr, 10);
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing address '" << hexAddr << "': " << e.what() << std::endl;
                address = 0;
            }

            proc->instructions.push_back(std::make_shared<WriteInstruction>(address, varName));
        }
        else if (cmd == "READ") {
            std::string varName, hexAddr;
            ss >> varName >> hexAddr;

            int address;
            try {
                if (hexAddr.substr(0, 2) == "0x" || hexAddr.substr(0, 2) == "0X") {
                    address = std::stoi(hexAddr, nullptr, 16);
                }
                else {
                    try {
                        address = std::stoi(hexAddr, nullptr, 16);
                    }
                    catch (...) {
                        address = std::stoi(hexAddr, nullptr, 10);
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing address '" << hexAddr << "': " << e.what() << std::endl;
                address = 0;
            }

            proc->instructions.push_back(std::make_shared<ReadInstruction>(varName, address));
        }
        else if (trimmed.find("PRINT(") == 0) {
            // Handle PRINT("message" + variable) format
            size_t openParen = trimmed.find('(');
            size_t closeParen = trimmed.rfind(')');

            if (openParen != std::string::npos && closeParen != std::string::npos) {
                std::string content = trimmed.substr(openParen + 1, closeParen - openParen - 1);

                // Debug: see what content we extracted
                std::cout << "DEBUG: PRINT content extracted: '" << content << "'" << std::endl;

                proc->instructions.push_back(std::make_shared<UserPrintInstruction>(content));
            }
        }
    }

    return proc;
}


