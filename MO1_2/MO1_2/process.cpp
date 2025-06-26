#include "process.h"
#include <random>
#include <sstream>
#include <stdexcept>

std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns) {
    auto proc = std::make_shared<Process>();
    proc->pid = pid;
    proc->name = name;
    proc->totalInstructions = 0;
    proc->instructionPointer = 0;
    proc->coreAssigned = -1;
    proc->isRunning = false;
    proc->isFinished = false;
    proc->completedInstructions = std::make_shared<std::atomic<int>>(0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instructionCount(minIns, maxIns);
    std::uniform_int_distribution<> opcodePicker(0, 5);
    std::uniform_int_distribution<> valueDist(1, 100);
    std::uniform_int_distribution<> sleepTime(100, 1000);
    std::uniform_int_distribution<> loopCountDist(2, 5);

    try {
        int numInstructions = instructionCount(gen);
        proc->totalInstructions = numInstructions;

        for (int i = 0; i < numInstructions; ++i) {
            Instruction inst;
            int op = opcodePicker(gen);
            switch (op) {
            case 0:
                inst.opcode = "DECLARE";
                inst.arg1 = "var" + std::to_string(i);
                inst.arg2 = std::to_string(valueDist(gen));
                break;
            case 1:
                inst.opcode = "ADD";
                inst.arg1 = "result" + std::to_string(i);
                inst.arg2 = "var" + std::to_string(i % 5);
                inst.arg3 = "var" + std::to_string((i + 1) % 5);
                break;
            case 2:
                inst.opcode = "SUBTRACT";
                inst.arg1 = "result" + std::to_string(i);
                inst.arg2 = "var" + std::to_string(i % 5);
                inst.arg3 = "var" + std::to_string((i + 1) % 5);
                break;
            case 3:
                inst.opcode = "PRINT";
                inst.arg1 = "Hello from " + name + " [" + std::to_string(i) + "]";
                break;
            case 4:
                inst.opcode = "SLEEP";
                inst.arg1 = std::to_string(sleepTime(gen)); // milliseconds
                break;
            case 5:
                inst.opcode = "FOR";
                inst.loopCount = loopCountDist(gen);
                for (int j = 0; j < 3; ++j) {
                    Instruction subInst;
                    subInst.opcode = "PRINT";
                    subInst.arg1 = "Loop iteration " + std::to_string(j + 1);
                    inst.subInstructions.push_back(subInst);
                }
                break;
            }
            proc->instructions.push_back(inst);
        }
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Process generation failed: " << e.what();
        proc->logs.push_back(oss.str());
    }

    return proc;
}
