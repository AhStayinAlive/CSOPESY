#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <memory>

class Process; // Forward declaration only

class Instruction {
public:
    virtual ~Instruction() = default;
    virtual void execute(std::shared_ptr<Process> proc, int coreId) = 0;
};

#endif