#include "WriteInstruction.h"
#include "MemoryManager.h"
#include "utils.h"
#include "Process.h"
#include <stdexcept>
#include <string>
#include <sstream>

WriteInstruction::WriteInstruction(size_t addr, uint16_t val)
    : address(addr), value(val) {
}

void WriteInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto& memMgr = MemoryManager::getInstance();

    try {
        // Use the MemoryManager's write method
        memMgr.write(proc, address, value);

        // Log the write operation
        std::ostringstream logEntry;
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId << " | PID " << proc->pid
            << " | WRITE: Address " << to_hex(address)
            << " = " << value;

        proc->logs.push_back(logEntry.str());
        logToFile(proc->name, logEntry.str(), coreId);

    }
    catch (const std::exception& e) {
        std::ostringstream errorLog;
        errorLog << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId << " | PID " << proc->pid
            << " | WRITE ERROR: " << e.what();

        proc->logs.push_back(errorLog.str());
        logToFile(proc->name, errorLog.str(), coreId);
        throw; // Re-throw the exception
    }
}