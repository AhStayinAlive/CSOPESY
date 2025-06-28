#include "PrintInstruction.h"
#include "process.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdint>

PrintInstruction::PrintInstruction(const std::string& msg)
    : message(msg), hasVariable(false) {
}

PrintInstruction::PrintInstruction(const std::string& textPart, const std::string& varName)
    : message(textPart), variableName(varName), hasVariable(true) {
}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId, int currentDepth) {
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

    // ❌ Remove this line:
    // std::cout << output << std::endl;

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | " << std::string(currentDepth * 2, ' ')
        << message;

    std::string finalLog = logEntry.str();
    proc->logs.push_back(finalLog);
    logToFile(proc->name, finalLog, coreId);
}
