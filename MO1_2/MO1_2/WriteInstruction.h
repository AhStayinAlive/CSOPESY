#ifndef WRITEINSTRUCTION_H
#define WRITEINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class WriteInstruction : public Instruction {
private:
    int address;
    std::string variableName;
    std::string logPrefix = "";

public:
    WriteInstruction(int addr, const std::string& varName, const std::string& logPrefix = "");
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};

#endif