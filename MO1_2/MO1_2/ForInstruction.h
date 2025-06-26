// ForInstruction.h
#pragma once
#include "instruction.h"
#include <vector>

class ForInstruction : public Instruction {
    int loopCount;
    std::vector<std::shared_ptr<Instruction>> subInstructions;

public:
    ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& body);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};