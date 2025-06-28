#ifndef DECLAREINSTRUCTION_H
#define DECLAREINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>
#include <cstdint>

class DeclareInstruction : public Instruction {
private:
    std::string variableName;
    uint16_t value;

public:
    DeclareInstruction(const std::string& varName, int val);
    void execute(std::shared_ptr<Process> proc, int coreId = -1, int currentDepth = 0) override;

    std::string getVariableName() const { return variableName; }
    uint16_t getValue() const { return value; }
};

#endif // DECLAREINSTRUCTION_H
