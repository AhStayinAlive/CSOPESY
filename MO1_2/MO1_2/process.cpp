#include "process.h"
#include "config.h"
#include "scheduler.h"
#include "utils.h"
#include <cstdlib>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <chrono>
#include <fstream>
#include <unordered_map>
#include <algorithm>

std::shared_ptr<Process> generateRandomProcess(int pid, int minIns, int maxIns) {
    std::shared_ptr<Process> p = std::make_shared<Process>();
    int count = minIns + (std::rand() % (maxIns - minIns + 1));

    p->pid = pid;
    std::stringstream ss;
    ss << "p" << (pid < 10 ? "0" : "") << pid;
    p->name = ss.str();
    p->instructionPointer = 0;
    p->totalInstructions = count;
    p->coreAssigned = -1;
    p->isRunning = false;
    p->isFinished = false;
    p->completedInstructions = std::make_shared<std::atomic<int>>(0);

    for (int i = 0; i < count; ++i) {
        Instruction ins;
        int opcode = rand() % 5;

        if (opcode == 0) {
            ins.opcode = "PRINT";
            ins.arg1 = "Hello world from " + p->name + "!";
        }
        else if (opcode == 1) {
            ins.opcode = "SLEEP";
            ins.arg1 = std::to_string(rand() % 3 + 1);
        }
        else if (opcode == 2) {
            ins.opcode = "ADD";
            ins.arg1 = "result";
            ins.arg2 = (rand() % 2 == 0) ? "x" : "2";
            ins.arg3 = (rand() % 2 == 0) ? "y" : "3";
        }
        else if (opcode == 3) {
            ins.opcode = "SUBTRACT";
            ins.arg1 = "diff";
            ins.arg2 = (rand() % 2 == 0) ? "10" : "a";
            ins.arg3 = (rand() % 2 == 0) ? "b" : "5";
        }
        else if (opcode == 4) {
            ins.opcode = "DECLARE";
            ins.arg1 = "var" + std::to_string(rand() % 5 + 1);
            ins.arg2 = std::to_string(rand() % 50 + 1);
        }

        p->instructions.push_back(ins);
    }

    std::ofstream out(p->name + ".txt", std::ios::out);
    if (out) {
        out << "Process name: " << p->name << std::endl;
        out << "Process ID: " << p->pid << std::endl;
        out << "Logs:" << std::endl << std::endl;
        out.close();
    }

    return p;
}
