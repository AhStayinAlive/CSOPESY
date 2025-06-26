#include "PrintInstruction.h"
#include "utils.h"
#include "process.h"
#include <sstream>

PrintInstruction::PrintInstruction(const std::string& msg) : message(msg) {}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string timestamp = getCurrentTimestamp();
    std::ostringstream logEntry;

    logEntry << "[" << timestamp << "] "
        << "Core " << coreId
        << " | PID " << proc->pid
        << " | PRINT: \"" << message << "\"";

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
