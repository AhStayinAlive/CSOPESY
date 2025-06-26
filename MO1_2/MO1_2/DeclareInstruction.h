
// DeclareInstruction.h
#pragma once
#include "instruction.h"
#include <string>

class DeclareInstruction : public Instruction {
    std::string varName;
    int value;

public:
    DeclareInstruction(const std::string& name, int val);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};