// ForInstruction.h
#pragma once
#include "instruction.h"
#include <vector>
#include <memory>

class ForInstruction : public Instruction {
private:
    int iterations;
    std::vector<std::shared_ptr<Instruction>> subInstructions;

public:
    ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    int getIterations() const { return iterations; }
};

