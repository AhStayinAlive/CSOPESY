#include "DeclareInstruction.h"
#include "process.h"
#include "utils.h"
#include <algorithm>
#include <sstream>
#include <cstdint>


DeclareInstruction::DeclareInstruction(const std::string& varName, int val)
    : variableName(varName), value(static_cast<uint16_t>(std::clamp(val, 0, 65535))) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    proc->memory[variableName] = value;

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | DECLARE: " << variableName << " = " << value;

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
