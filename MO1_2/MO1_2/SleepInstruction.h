#pragma once
#include "instruction.h"

class SleepInstruction : public Instruction {
    int sleepMs;

public:
    explicit SleepInstruction(int ms);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};
