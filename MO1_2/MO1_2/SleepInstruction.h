#pragma once
#include "instruction.h"
#include "process.h"
#include <memory>

class SleepInstruction : public Instruction {
private:
    int duration;  // Duration in milliseconds
    

public:
    std::string logPrefix = "";
    SleepInstruction(int ms, const std::string& logPrefix = "");
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    int getDuration() const { return duration; }
};
