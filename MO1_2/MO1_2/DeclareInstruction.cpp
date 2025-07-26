#include "DeclareInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <sstream>

DeclareInstruction::DeclareInstruction(const std::string& varName, int val, const std::string& logPrefix)
    : variableName(varName), value(val), logPrefix(logPrefix) {
}

void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    size_t frameSize = MemoryManager::getInstance().getFrameSize();
    size_t memSize = std::max<size_t>(1, proc->memory.size()); // Avoid zero
    size_t hashVal = std::hash<std::string>{}(variableName);
    size_t page = (hashVal % memSize) / frameSize;

    if (!proc->loadedPages.count(page)) {
        MemoryManager::getInstance().handlePageFault(proc, page);
    }

    
    proc->memory[variableName] = value;
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | DECLARE: " << variableName << " = " << value;
    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
