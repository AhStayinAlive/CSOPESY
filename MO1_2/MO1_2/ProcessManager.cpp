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
#include <unordered_map>
#include "config.h"

static std::vector<std::shared_ptr<Process>> allProcesses;
static std::unordered_map<std::string, std::shared_ptr<Process>> processMap;

std::shared_ptr<Process> ProcessManager::createProcess(const std::string& name, int pid, int minInstructions, int maxInstructions) {
    auto proc = std::make_shared<Process>();
    proc->pid = pid;
    proc->name = name;
    proc->instructionPointer = 0;
    proc->coreAssigned = -1;
    proc->isRunning = false;
    proc->isFinished = false;
    proc->completedInstructions = std::make_shared<std::atomic<int>>(0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instructionCount(minInstructions, maxInstructions);
    std::uniform_int_distribution<> opcodePicker(0, 5);
    std::uniform_int_distribution<> valueDist(1, 100);
    std::uniform_int_distribution<> sleepTime(100, 500);
    std::uniform_int_distribution<> loopCountDist(2, 4);

    int numInstructions = instructionCount(gen);
    proc->totalInstructions = numInstructions;

    for (int i = 0; i < 5; ++i) {
        proc->instructions.push_back(std::make_shared<DeclareInstruction>("var" + std::to_string(i), valueDist(gen)));
    }

    for (int i = 5; i < numInstructions; ++i) {
        int op = opcodePicker(gen);
        switch (op) {
        case 0:
            proc->instructions.push_back(std::make_shared<DeclareInstruction>("var" + std::to_string(i), valueDist(gen)));
            break;
        case 1:
            proc->instructions.push_back(std::make_shared<AddInstruction>(
                "sum" + std::to_string(i), "var" + std::to_string(i % 5), "var" + std::to_string((i + 1) % 5)));
            break;
        case 2:
            proc->instructions.push_back(std::make_shared<SubtractInstruction>(
                "diff" + std::to_string(i), "var" + std::to_string(i % 5), "var" + std::to_string((i + 1) % 5)));
            break;
        case 3:
            proc->instructions.push_back(std::make_shared<PrintInstruction>("Message from " + name + " @" + std::to_string(i)));
            break;
        case 4:
            proc->instructions.push_back(std::make_shared<SleepInstruction>(sleepTime(gen)));
            break;
        case 5: {
            int loopCount = loopCountDist(gen);
            std::vector<std::shared_ptr<Instruction>> subInsts;
            for (int j = 0; j < 3; ++j) {
                subInsts.push_back(std::make_shared<PrintInstruction>("Loop " + std::to_string(j + 1)));
            }
            proc->instructions.push_back(std::make_shared<ForInstruction>(loopCount, subInsts));
            break;
        }
        }
    }

    allProcesses.push_back(proc);
    processMap[name] = proc;
    return proc;
}

std::shared_ptr<Process> ProcessManager::createUniqueNamedProcess(int minIns, int maxIns) {
    static int id = 1;
    return createProcess("P" + std::to_string(id++), id, minIns, maxIns);
}

std::shared_ptr<Process> ProcessManager::createNamedProcess(const std::string& name) {
    static int pid = 1000;
    return createProcess(name, pid++, Config::getInstance().minInstructions, Config::getInstance().maxInstructions);
}

std::shared_ptr<Process> ProcessManager::findByName(const std::string& name) {
    auto it = processMap.find(name);
    return (it != processMap.end()) ? it->second : nullptr;
}

void ProcessManager::addProcess(std::shared_ptr<Process> proc) {
    allProcesses.push_back(proc);
    processMap[proc->name] = proc;
}

std::vector<std::shared_ptr<Process>> ProcessManager::getAllProcesses() {
    return allProcesses;
}
