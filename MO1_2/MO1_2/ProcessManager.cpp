#include "ProcessManager.h"
#include "DeclareInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "PrintInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include <random>
#include <sstream>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include "config.h"

static std::vector<std::shared_ptr<Process>> allProcesses;
static std::unordered_map<std::string, std::shared_ptr<Process>> processMap;
static int pidCounter = 1000;
static int uniqueProcessCounter = 1;

std::shared_ptr<Process> ProcessManager::createProcess(const std::string& name, int pid, int minInstructions, int maxInstructions) {
    auto proc = std::make_shared<Process>();
    proc->pid = pid;
    proc->name = name;
    proc->instructionPointer = 0;
    proc->coreAssigned = -1;
    proc->isRunning = false;
    proc->isFinished = false;
    proc->isDetached = false;
    proc->completedInstructions = std::make_shared<std::atomic<int>>(0);

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

        for (int i = 0; i < 5; ++i) {
            std::string varName = "var" + std::to_string(i);
            int value = valueDist(gen);
            proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));
        }

        int i = 0;

        std::function<std::vector<std::shared_ptr<Instruction>>(int, int)> generateRandomSubInstructions;

        generateRandomSubInstructions = [&](int currentDepth, int maxDepth) -> std::vector<std::shared_ptr<Instruction>> {
            std::vector<std::shared_ptr<Instruction>> subInstrs;
            std::uniform_int_distribution<> subOpcodePicker(0, (currentDepth < maxDepth ? 5 : 4));  // Allow nested FOR if not max depth
            std::uniform_int_distribution<> subInstrCountDist(1, 3); // 1 to 3 subinstructions
            int numSubInstr = subInstrCountDist(gen);

            for (int j = 0; j < numSubInstr; ++j) {
                int subOp = subOpcodePicker(gen);
                switch (subOp) {
                case 0: { // DECLARE
                    std::string varName = "loopVar_" + std::to_string(currentDepth) + "_" + std::to_string(j);
                    int val = valueDist(gen);
                    subInstrs.push_back(std::make_shared<DeclareInstruction>(varName, val));
                    break;
                }
                case 1: { // ADD
                    subInstrs.push_back(std::make_shared<AddInstruction>("res", "var0", "var1"));
                    break;
                }
                case 2: { // SUBTRACT
                    subInstrs.push_back(std::make_shared<SubtractInstruction>("res", "var0", "var1"));
                    break;
                }
                case 3: { // PRINT
                    subInstrs.push_back(std::make_shared<PrintInstruction>("Nested print at depth " + std::to_string(currentDepth)));
                    break;
                }
                case 4: { // SLEEP
                    int ms = sleepTime(gen);
                    subInstrs.push_back(std::make_shared<SleepInstruction>(ms));
                    break;
                }
                case 5: { // Nested FOR
                    int nestedLoopCount = loopCountDist(gen);
                    auto nestedSubInstrs = generateRandomSubInstructions(currentDepth + 1, maxDepth);
                    subInstrs.push_back(std::make_shared<ForInstruction>(nestedLoopCount, nestedSubInstrs));
                    break;
                }
                }
            }

            return subInstrs;
            };


        while (proc->instructions.size() < numInstructions) {
            int op = opcodePicker(gen);
            switch (op) {
            case 0: {
                std::string varName = "var" + std::to_string(i);
                int value = valueDist(gen);
                proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));
                break;
            }
            case 1: {
                std::string result = "sum" + std::to_string(i);
                std::string lhs = "var" + std::to_string(i % 5);
                std::string rhs = "var" + std::to_string((i + 1) % 5);
                proc->instructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs));
                break;
            }
            case 2: {
                std::string result = "diff" + std::to_string(i);
                std::string lhs = "var" + std::to_string(i % 5);
                std::string rhs = "var" + std::to_string((i + 1) % 5);
                proc->instructions.push_back(std::make_shared<SubtractInstruction>(result, lhs, rhs));
                break;
            }
            case 3: {
                std::string message = "Hello from " + name + " [instruction " + std::to_string(i) + "]";
                proc->instructions.push_back(std::make_shared<PrintInstruction>(message));
                break;
            }
            case 4: {
                int duration = sleepTime(gen);
                proc->instructions.push_back(std::make_shared<SleepInstruction>(duration));
                break;
            }
            case 5: {
                int loopCount = loopCountDist(gen);
                auto subInstructions = generateRandomSubInstructions(1, 3); // currentDepth=1, maxDepth=3
                proc->instructions.push_back(std::make_shared<ForInstruction>(loopCount, subInstructions));
                break;
            }


            }

            i++;
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
    return createProcess(processName, pidCounter++, minIns, maxIns);
}

std::shared_ptr<Process> ProcessManager::createNamedProcess(const std::string& name) {
    if (processMap.find(name) != processMap.end()) {
        return processMap[name];
    }
    const auto& config = Config::getInstance();
    return createProcess(name, pidCounter++, config.minInstructions, config.maxInstructions);
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



