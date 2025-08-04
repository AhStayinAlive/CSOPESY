#include "DeclareInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <sstream>
#include <cstdint>


DeclareInstruction::DeclareInstruction(
    const std::string& varName, 
    int val, 
    const std::string& logPrefix)
    : variableName(varName), value(static_cast<uint16_t>(std::clamp(val, 0, 65535))), logPrefix(logPrefix) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[variableName] = value;

    int address = MemoryManager::getInstance().allocateVariable(proc, variableName);
    MemoryManager::getInstance().write(proc, address, value);

    std::ostringstream logEntry;

    if (logPrefix != "") {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | DECLARE: " << variableName << " = " << value;
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | DECLARE: " << variableName << " = " << value;
    }

    

    

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), proc->coreAssigned);
}
