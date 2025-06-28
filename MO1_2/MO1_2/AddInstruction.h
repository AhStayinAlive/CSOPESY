#pragma once
#include "instruction.h"
#include <string>
#include <memory>

struct Process;

class AddInstruction : public Instruction {
public:
    std::string resultVar;
    std::string arg1;
    std::string arg2;

    AddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs);
    void execute(std::shared_ptr<Process> proc, int coreId, int currentDepth = 0) override;
};