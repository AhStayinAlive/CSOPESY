#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <memory>

struct Process;

class Instruction {
public:
    virtual ~Instruction() = default;
    virtual void execute(std::shared_ptr<Process> proc, int coreId) = 0;
};

#endif
