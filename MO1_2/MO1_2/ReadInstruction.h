#ifndef READINSTRUCTION_H
#define READINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class ReadInstruction : public Instruction {
private:
    std::string variableName;
    int address;
    std::string logPrefix = "";

public:
    ReadInstruction(const std::string& varName, int addr, const std::string& logPrefix = "");
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};

#endif#pragma once
