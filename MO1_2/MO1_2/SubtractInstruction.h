#ifndef SUBTRACTINSTRUCTION_H
#define SUBTRACTINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class SubtractInstruction : public Instruction {
public:
    std::string resultVar;
    std::string arg1;
    std::string arg2;
    std::string logPrefix;

    // 3-parameter constructor (for ProcessManager compatibility)
    SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs);

    // 4-parameter constructor (for cases where logPrefix is specified)
    SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix);

    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;
};

#endif