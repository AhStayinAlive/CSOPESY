#include "AddInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <sstream>
#include <stdexcept>

AddInstruction::AddInstruction(const std::string& result,
    const std::string& lhs,
    const std::string& rhs,
    const std::string& logPrefix)
    : resultVar(result), arg1(lhs), arg2(rhs), logPrefix(logPrefix) {
}

void AddInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto& memMgr = MemoryManager::getInstance();

    // Helper to read uint16_t from memory
    auto readFromMemory = [&](size_t addr) -> uint16_t {
        size_t page = addr / memMgr.getFrameSize();
        if (!proc->loadedPages.count(page)) {
            memMgr.handlePageFault(proc, page);
        }
        uint16_t value = static_cast<uint16_t>(
            static_cast<unsigned char>(proc->memory[addr]) |
            (static_cast<unsigned char>(proc->memory[addr + 1]) << 8)
            );
        return value;
        };

    // Helper to write uint16_t to memory
    auto writeToMemory = [&](size_t addr, uint16_t value) {
        size_t page = addr / memMgr.getFrameSize();
        if (!proc->loadedPages.count(page)) {
            memMgr.handlePageFault(proc, page);
        }
        proc->memory[addr] = value & 0xFF;
        proc->memory[addr + 1] = (value >> 8) & 0xFF;
        };

    // Helper to resolve variable or immediate value
    auto getValue = [&](const std::string& name) -> uint16_t {
        if (proc->variableAddressMap.count(name)) {
            size_t addr = proc->variableAddressMap[name];
            return readFromMemory(addr);
        }
        try {
            return static_cast<uint16_t>(std::stoi(name));
        }
        catch (...) {
            return 0;
        }
        };

    // Fetch operands
    uint16_t val1 = getValue(arg1);
    uint16_t val2 = getValue(arg2);
    uint16_t result = val1 + val2;

    // Allocate address if result variable is new
    if (proc->variableAddressMap.count(resultVar) == 0) {
        if (proc->variableAddressMap.size() >= 32) {
            throw std::runtime_error("Symbol table full");
        }
        size_t newAddr = proc->variableAddressMap.size() * 2;
        proc->variableAddressMap[resultVar] = newAddr;
    }

    size_t resultAddr = proc->variableAddressMap[resultVar];
    writeToMemory(resultAddr, result);

    // Logging
    std::ostringstream logEntry;
    logEntry << "[" << getCurrentTimestamp() << "] "
        << "Core " << coreId << " | PID " << proc->pid
        << " | ADD: " << arg1 << "=" << val1 << " + "
        << arg2 << "=" << val2 << " = " << result
        << " (" << resultVar << ")";

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}
