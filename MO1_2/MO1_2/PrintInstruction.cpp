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

    if (hasVariable) {
        size_t addr = std::hash<std::string>{}(variableName) % proc->memory.size();
        size_t virtualPage = addr / MemoryManager::getInstance().getFrameSize();

        if (!proc->loadedPages.count(virtualPage)) {
            std::string var;
            int val;
            if (MemoryManager::getInstance().loadFromBackingStore(proc, virtualPage, var, val)) {
                size_t varAddr = std::hash<std::string>{}(var) % proc->memory.size();
                proc->memory[varAddr] = static_cast<char>(val);
            }
            else {
                // First time page use — initialize
                size_t varAddr = std::hash<std::string>{}(var) % proc->memory.size();
                proc->memory[varAddr] = static_cast<char>(val);
                MemoryManager::getInstance().writeToBackingStore(proc, virtualPage, var, val);
            }
            proc->loadedPages.insert(virtualPage);
        }

        logEntry << message << static_cast<int>(proc->memory[addr]);
    }
    else {
        logEntry << message;
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}

