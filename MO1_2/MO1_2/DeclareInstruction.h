#pragma once
#include "Instruction.h"
#include <string>

class DeclareInstruction : public Instruction {
private:
    std::string varName;
    uint16_t value;

public:
    DeclareInstruction(const std::string& var, uint16_t val);  // <--- make sure this line exists

    void execute(std::shared_ptr<Process> proc, int coreId) override;
};
