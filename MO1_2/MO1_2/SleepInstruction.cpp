#pragma once
#include "instruction.h"
#include "process.h"
#include "utils.h"
#include <memory>

class SleepInstruction : public Instruction {
    int duration;

public:
    SleepInstruction(int ms) : duration(ms) {}

    void execute(std::shared_ptr<Process> proc, int coreId = -1, int currentCpuTick = 0) {
        int delayTicks = duration / 100; // 1 tick = 100ms
        int targetTick = currentCpuTick + delayTicks;

        proc->setWakeupTick(targetTick);

        logToFile(proc->name, "SLEEP for " + std::to_string(duration) + "ms (wakeup at tick " + std::to_string(targetTick) + ")", coreId);
    }

};
