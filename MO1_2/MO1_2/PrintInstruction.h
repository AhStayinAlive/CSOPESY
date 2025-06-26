#pragma once
#include "instruction.h"
#include <string>
#include <memory>

class PrintInstruction : public Instruction {
    std::string message;

public:
    PrintInstruction(const std::string& msg);
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    std::string getMessage() const { return message; }
};
