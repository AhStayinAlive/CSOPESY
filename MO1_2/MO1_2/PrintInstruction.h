#pragma once
#include "instruction.h"
#include <string>

class PrintInstruction : public Instruction {
    std::string message;
    bool isVariable;

public:
    PrintInstruction(const std::string& msg, bool isVar = false);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};

