#pragma once
#ifndef USERPRINTINSTRUCTION_H
#define USERPRINTINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class UserPrintInstruction : public Instruction {
private:
    std::string message;
    std::string logPrefix = "";

public:
    UserPrintInstruction(const std::string& msg, const std::string& logPrefix = "");
    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};

#endif