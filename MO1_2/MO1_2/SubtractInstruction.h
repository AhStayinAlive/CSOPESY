#ifndef SUBTRACTINSTRUCTION_H
#define SUBTRACTINSTRUCTION_H

#pragma once
#include "instruction.h"
#include <string>
#include <memory>

class SubtractInstruction : public Instruction {
public:
    std::string resultVar;
    std::string arg1;
    std::string arg2;

    // Constructor
    SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs);

    // Override execute method
    void execute(std::shared_ptr<Process> proc, int coreId = -1, int currentDepth = 0) override;
};

#endif // SUBTRACTINSTRUCTION_H
