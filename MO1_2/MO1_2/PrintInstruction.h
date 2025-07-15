#ifndef PRINTINSTRUCTION_H
#define PRINTINSTRUCTION_H

#include "instruction.h"
#include <string>
#include <memory>

class PrintInstruction : public Instruction {
private:
    std::string message;
    std::string variableName;
    bool hasVariable = false;
    std::string logPrefix = "";

public:
    // Constructor for static message
    // Constructor for static message
    PrintInstruction(const std::string& msg, const std::string& logPrefix = "");
    // Add this third constructor (note: bool withVariable breaks ambiguity)
    PrintInstruction(const std::string& messagePart, const std::string& variableName, bool withVariable, const std::string& logPrefix = "");

    // Constructor for message + variable (no default for logPrefix to avoid overload ambiguity)
    PrintInstruction(const std::string& messagePart, const std::string& variableName, const std::string& logPrefix);


    // Constructor for message + variable
    //PrintInstruction(const std::string& textPart, const std::string& varName, const std::string& logPrefix = "");

    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    std::string getMessage() const { return message; }
    std::string getVariableName() const { return variableName; }
    bool getHasVariable() const { return hasVariable; }
};

#endif