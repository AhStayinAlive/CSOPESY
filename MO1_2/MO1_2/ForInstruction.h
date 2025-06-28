#ifndef FORINSTRUCTION_H
#define FORINSTRUCTION_H

#include "instruction.h"
#include <vector>
#include <memory>

class ForInstruction : public Instruction {
private:
    int iterations;
    std::vector<std::shared_ptr<Instruction>> subInstructions;
    int nestingLevel;

public:
    ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions, int nesting = 1);
    void execute(std::shared_ptr<Process> proc, int coreId) override;
    int getIterations() const { return iterations; }
    int getNestingLevel() const { return nestingLevel; }
};

#endif
