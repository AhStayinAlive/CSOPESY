// PrintInstruction.h
#pragma once
#include "instruction.h"
#include <string>

class PrintInstruction : public Instruction {
    std::string message;

public:
    explicit PrintInstruction(const std::string& msg);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};