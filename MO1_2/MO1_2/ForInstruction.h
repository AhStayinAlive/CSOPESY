// ForInstruction.h
#ifndef FORINSTRUCTION_H
#define FORINSTRUCTION_H

#include "instruction.h"
#include <vector>
#include <memory>

class ForInstruction : public Instruction {
private:
    int iterations;
    int loopCount;
    std::vector<std::shared_ptr<Instruction>> subInstructions;
    int nestingLevel; // Track nesting depth

public:
    ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& subs);
    void execute(std::shared_ptr<Process> proc, int coreId, int currentDepth = 0) override;
    int getIterations() const { return iterations; }
    int getNestingLevel() const { return nestingLevel; }
};

#endif