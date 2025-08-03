#include "SubtractInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <sstream>
#include <stdexcept>
// 3-parameter constructor
SubtractInstruction::SubtractInstruction(const std::string& result,
    const std::string& lhs,
    const std::string& rhs)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix("") {
}

// 4-parameter constructor
SubtractInstruction::SubtractInstruction(const std::string& result,
    const std::string& lhs,
    const std::string& rhs,
    const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}
void SubtractInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto& memMgr = MemoryManager::getInstance();

    // Validate symbol table space
    if (proc->variableAddressMap.size() >= 32 &&
        proc->variableAddressMap.count(resultVar) == 0) {
        throw std::runtime_error("Symbol table full (max 32 variables)");
    }

    // Helper function to get variable value using MemoryManager
    auto getValue = [&](const std::string& name) -> uint16_t {
        if (proc->variableAddressMap.count(name)) {
            size_t addr = proc->variableAddressMap[name];
            try {
                return memMgr.read(proc, addr);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Access violation for " + name + ": " + e.what());
            }
        }
        try {
            return static_cast<uint16_t>(std::stoi(name)); // Handle literals
        }
        catch (...) {
            return 0;
        }
        };

    // Perform subtraction
    uint16_t val1 = getValue(arg1);
    uint16_t val2 = getValue(arg2);
    uint16_t result = (val1 > val2) ? (val1 - val2) : 0;

    // Store result
    if (proc->variableAddressMap.count(resultVar) == 0) {
        // Assign new address in symbol table (0x0000-0x0040)
        size_t newAddr = 0x0000 + (proc->variableAddressMap.size() * 2);
        proc->variableAddressMap[resultVar] = newAddr;
    }
    size_t addr = proc->variableAddressMap[resultVar];

    // Use MemoryManager to write the result consistently
    try {
        memMgr.write(proc, addr, result);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to store result for " + resultVar + ": " + e.what());
    }

    // Logging
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | SUB: " << arg1 << "=" << val1 << " - "
        << arg2 << "=" << val2 << " = " << result
        << " (" << resultVar << ")";

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}