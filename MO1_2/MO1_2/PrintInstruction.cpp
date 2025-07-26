#include "PrintInstruction.h"
#include "process.h"
#include "MemoryManager.h"
#include "utils.h"
#include <sstream>

PrintInstruction::PrintInstruction(const std::string& msg, const std::string& logPrefix)
    : message(msg), hasVariable(false), logPrefix(logPrefix) {
}

PrintInstruction::PrintInstruction(const std::string& messagePart, const std::string& variableName, bool withVariable, const std::string& logPrefix)
    : message(messagePart), variableName(variableName), hasVariable(withVariable), logPrefix(logPrefix) {
}

PrintInstruction::PrintInstruction(const std::string& messagePart, const std::string& variableName, const std::string& logPrefix)
    : message(messagePart), variableName(variableName), hasVariable(true), logPrefix(logPrefix) {
}

void PrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid << " | PRINT: ";

    if (hasVariable && proc->memory.find(variableName) != proc->memory.end()) {
        size_t virtualPage = std::hash<std::string>{}(variableName) % proc->memory.size() / MemoryManager::getInstance().getFrameSize();
        if (!proc->loadedPages.count(virtualPage)) {
            std::string var;
            int val;
            if (MemoryManager::getInstance().loadFromBackingStore(proc, virtualPage, var, val)) {
                proc->memory[var] = val; // Load from disk into memory
            }
            else {
                // First time this page is being used — initialize it
                proc->memory[var] = val;
                MemoryManager::getInstance().writeToBackingStore(proc, virtualPage, var, val);
            }
            proc->loadedPages.insert(virtualPage); // Mark page as loaded
        }
        
        logEntry << message << proc->memory[variableName];
    }
    else {
        logEntry << message;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
