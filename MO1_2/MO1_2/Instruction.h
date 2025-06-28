#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#pragma once
#include <memory>

struct Process; // Forward declaration only

class Instruction {
public:
    virtual ~Instruction() = default;
    virtual void execute(std::shared_ptr<Process> proc, int coreId, int currentDepth = 0) = 0;
};

#endif