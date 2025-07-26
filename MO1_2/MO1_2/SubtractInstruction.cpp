#include "SubtractInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <sstream>

SubtractInstruction::SubtractInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {}

void SubtractInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    if (!arg1.empty()) {
        size_t page1 = std::hash<std::string>{}(arg1) % proc->memory.size() / MemoryManager::getInstance().getFrameSize();
        if (!proc->loadedPages.count(page1)) {
            std::string var;
            int val;
            if (MemoryManager::getInstance().loadFromBackingStore(proc, page1, var, val)) {
                proc->memory[var] = val; // Load from disk into memory
            }
            else {
                // First time this page is being used — initialize it
                proc->memory[var] = val;
                MemoryManager::getInstance().writeToBackingStore(proc, page1, var, val);
            }
            proc->loadedPages.insert(page1); // Mark page as loaded
        }
    }
    if (!arg2.empty()) {
        size_t page2 = std::hash<std::string>{}(arg2) % proc->memory.size() / MemoryManager::getInstance().getFrameSize();
        if (!proc->loadedPages.count(page2)) {
            std::string var;
            int val;
            if (MemoryManager::getInstance().loadFromBackingStore(proc, page2, var, val)) {
                proc->memory[var] = val; // Load from disk into memory
            }
            else {
                // First time this page is being used — initialize it
                proc->memory[var] = val;
                MemoryManager::getInstance().writeToBackingStore(proc, page2, var, val);
            }
            proc->loadedPages.insert(page2); // Mark page as loaded
        }
    }

    uint16_t val1 = proc->memory.count(arg1) ? proc->memory[arg1] : 0;
    uint16_t val2 = proc->memory.count(arg2) ? proc->memory[arg2] : 0;

    uint16_t result = (val1 > val2) ? (val1 - val2) : 0;
    proc->memory[resultVar] = result;

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
             << "Core " << coreId << " | PID " << proc->pid
             << " | SUBTRACT: " << val1 << " - " << val2 << " = " << result;

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
