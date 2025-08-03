#include "ReadInstruction.h"
#include "MemoryManager.h"
#include "Process.h"
#include "utils.h"
#include <stdexcept>
#include <string>
#include <sstream>

ReadInstruction::ReadInstruction(const std::string& var, size_t addr)
    : varName(var), address(addr) {
}

void ReadInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto& memMgr = MemoryManager::getInstance();

    try {
        // Use the MemoryManager's read method (already implemented in MemoryManager.cpp)
        uint16_t value = memMgr.read(proc, address);

        // Store the value in the variable if it exists in the symbol table
        if (proc->variableAddressMap.count(varName)) {
            size_t varAddr = proc->variableAddressMap[varName];
            memMgr.write(proc, varAddr, value);
        }
        else {
            // Create new variable entry in symbol table
            if (proc->variableAddressMap.size() >= 32) {
                throw std::runtime_error("Symbol table full (max 32 variables)");
            }
            size_t newAddr = 0x0000 + (proc->variableAddressMap.size() * 2);
            proc->variableAddressMap[varName] = newAddr;
            memMgr.write(proc, newAddr, value);
        }

        // Log the read operation
        std::ostringstream logEntry;
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId << " | PID " << proc->pid
            << " | READ: Address " << to_hex(address)
            << " = " << value << " -> " << varName;

        proc->logs.push_back(logEntry.str());
        logToFile(proc->name, logEntry.str(), coreId);

    }
    catch (const std::exception& e) {
        std::ostringstream errorLog;
        errorLog << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId << " | PID " << proc->pid
            << " | READ ERROR: " << e.what();

        proc->logs.push_back(errorLog.str());
        logToFile(proc->name, errorLog.str(), coreId);
        throw; // Re-throw the exception
    }
}

// REMOVE THIS - it's already implemented in MemoryManager.cpp
// This duplicate definition causes LNK2005 error
/*
uint16_t MemoryManager::read(std::shared_ptr<Process> proc, size_t address) {
    size_t page = address / frameSize;

    // Demand paging: load if not present
    if (!proc->loadedPages.count(page)) {
        handlePageFault(proc, page);
    }

    if (address + 1 >= proc->memory.size()) {
        throw std::runtime_error("Read out of bounds at " + std::to_string(address));
    }

    // Read 2 bytes from memory
    return static_cast<uint16_t>(proc->memory[address]) |
        (static_cast<uint16_t>(proc->memory[address + 1]) << 8);
}
*/