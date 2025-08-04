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
    PrintInstruction(const std::string& msg, const std::string& logPrefix = "");

    // Constructor for message + variable
    //PrintInstruction(const std::string& textPart, const std::string& varName, const std::string& logPrefix = "");

    void execute(std::shared_ptr<Process> proc, int coreId = -1) override;

    std::string getMessage() const { return message; }
    std::string getVariableName() const { return variableName; }
    bool getHasVariable() const { return hasVariable; }
};

#endif
