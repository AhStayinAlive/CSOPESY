// SubtractInstruction.h
#pragma once
#include "instruction.h"
#include <string>

class SubtractInstruction : public Instruction {
    std::string resultName, lhs, rhs;

public:
    std::string arg1;
    std::string arg2;
    std::string resultVar;
    SubtractInstruction(const std::string& res, const std::string& l, const std::string& r);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};