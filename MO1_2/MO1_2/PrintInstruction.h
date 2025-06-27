#ifndef PRINTINSTRUCTION_H
#define PRINTINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class PrintInstruction : public Instruction {
private:
    std::string message;         // Static text part
    std::string variableName;    // Optional variable name
    bool hasVariable = false;    // Indicates if variable should be appended

public:
    // Constructor for static message
    PrintInstruction(const std::string& msg);

    // Constructor for message + variable
    PrintInstruction(const std::string& textPart, const std::string& varName);

    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    std::string getMessage() const { return message; }
    std::string getVariableName() const { return variableName; }
    bool getHasVariable() const { return hasVariable; }
};

#endif // PRINTINSTRUCTION_H
