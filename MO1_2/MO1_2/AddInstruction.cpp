#include "AddInstruction.h"
#include "process.h"
#include "MemoryManager.h"
#include "utils.h"
#include <sstream>

AddInstruction::AddInstruction(const std::string& result, const std::string& lhs, const std::string& rhs, const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    uint16_t val1 = 0;
    uint16_t val2 = 0;

    if (proc->memory.find(arg1) != proc->memory.end()) { 
        size_t addr = proc->variableAddressMap[arg1];
        MemoryManager::getInstance().ensurePageLoaded(proc, addr);
        val1 = proc->memory[arg1]; 
    }
    else {
        try { val1 = static_cast<uint16_t>(std::stoi(arg1)); }
        catch (...) { val1 = 0; }
    }

    if (proc->memory.find(arg2) != proc->memory.end()) {
        size_t addr = proc->variableAddressMap[arg2];
        MemoryManager::getInstance().ensurePageLoaded(proc, addr);
        val2 = proc->memory[arg2];
    }
    else {
        try { val2 = static_cast<uint16_t>(std::stoi(arg2)); }
        catch (...) { val2 = 0; }
    }

    uint16_t sum = val1 + val2;
    size_t resultAddr = proc->variableAddressMap[resultVar];
    MemoryManager::getInstance().ensurePageLoaded(proc, resultAddr);
    proc->memory[resultVar] = sum;

    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | ADD: " << val1 << " + " << val2 << " = " << sum;
    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
