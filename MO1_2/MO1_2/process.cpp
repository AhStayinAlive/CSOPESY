#include "process.h"
#include "DeclareInstruction.h"
#include "PrintInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include <random>
#include <memory>
#include <sstream>

std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns) {
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
    std::uniform_int_distribution<> instructionCount(minIns, maxIns);
    std::uniform_int_distribution<> opcodePicker(0, 5);
    std::uniform_int_distribution<> valueDist(1, 100);
    std::uniform_int_distribution<> sleepTime(100, 1000);
    std::uniform_int_distribution<> loopCountDist(2, 5);

    try {
        int numInstructions = instructionCount(gen);
        proc->totalInstructions = numInstructions;

        // Always declare 5 initial variables
        for (int i = 0; i < 5; ++i) {
            std::string varName = "var" + std::to_string(i);
            int val = valueDist(gen);
            proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, val));
        }

        // Add random instructions
        for (int i = 5; i < numInstructions; ++i) {
            int op = opcodePicker(gen);

            switch (op) {
            case 0: { // DECLARE
                std::string varName = "var" + std::to_string(i);
                int val = valueDist(gen);
                proc->instructions.push_back(std::make_shared<DeclareInstruction>(varName, val));
                break;
            }
            case 1: { // ADD
                std::string result = "result" + std::to_string(i);
                std::string lhs = "var" + std::to_string(i % 5);
                std::string rhs = "var" + std::to_string((i + 1) % 5);
                proc->instructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs));
                break;
            }
            case 2: { // SUBTRACT
                std::string result = "result" + std::to_string(i);
                std::string lhs = "var" + std::to_string(i % 5);
                std::string rhs = "var" + std::to_string((i + 1) % 5);
                proc->instructions.push_back(std::make_shared<SubtractInstruction>(result, lhs, rhs));
                break;
            }
            case 3: { // PRINT
                std::string message = "Hello from " + name + " [" + std::to_string(i) + "]";
                proc->instructions.push_back(std::make_shared<PrintInstruction>(message));
                break;
            }
            case 4: { // SLEEP
                int ms = sleepTime(gen);
                proc->instructions.push_back(std::make_shared<SleepInstruction>(ms));
                break;
            }
            case 5: {
                int loopCount = loopCountDist(gen);
                for (int i = 0; i < loopCount; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        if (proc->instructions.size() >= maxIns) {
                            break; // Stop adding more instructions once max is reached
                        }
                        std::string msg = "Loop iteration " + std::to_string(j + 1) + " (outer " + std::to_string(i + 1) + ")";
                        proc->instructions.push_back(std::make_shared<PrintInstruction>(msg));
                    }
                    if (proc->instructions.size() >= maxIns) {
                        break;
                    }
                }
                break;
            }
            }
        }
    }
    catch (const std::exception& e) {
        proc->logs.push_back("Process generation failed: " + std::string(e.what()));
    }

    return proc;
}