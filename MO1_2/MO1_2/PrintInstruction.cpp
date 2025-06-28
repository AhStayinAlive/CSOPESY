#include "PrintInstruction.h"
#include "process.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdint>

PrintInstruction::PrintInstruction(const std::string& msg, const std::string& logPrefix)
    : message(msg), hasVariable(false), logPrefix(logPrefix) {
}

//PrintInstruction::PrintInstruction(const std::string& textPart, const std::string& varName, const std::string& logPrefix)
//    : message(textPart), variableName(varName), hasVariable(true), logPrefix(logPrefix) {
//}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string output = message;

    if (hasVariable) {
        uint16_t varValue = 0;
        if (proc->memory.find(variableName) != proc->memory.end()) {
            varValue = proc->memory[variableName];
        }
        else {
            proc->memory[variableName] = 0;  // auto-declare if missing
        }
        output += std::to_string(varValue);
    }

    std::ostringstream logEntry;

    if (logPrefix != "") {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | PRINT: \"" << output << "\"";
    }

    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | PRINT: \"" << output << "\"";
    }
    

    

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
