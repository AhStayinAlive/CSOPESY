#include "DeclareInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <sstream>

DeclareInstruction::DeclareInstruction(const std::string& varName, int val, const std::string& logPrefix)
    : variableName(varName), value(val), logPrefix(logPrefix) {
}

// Example for DeclareInstruction.cpp
void DeclareInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    auto& memMgr = MemoryManager::getInstance();

    if (proc->variableAddressMap.size() >= 32) {
        throw std::runtime_error("Symbol table full");
    }

    // Assign new address
    size_t newAddr = proc->variableAddressMap.size() * 2;
    proc->variableAddressMap[variableName] = newAddr;

    // Check if the page is loaded
    size_t page = newAddr / memMgr.getFrameSize();
    if (!proc->loadedPages.count(page)) {
        memMgr.handlePageFault(proc, page);
    }

    // Store value into memory (little-endian)
    proc->memory[newAddr] = static_cast<char>(value & 0xFF);            // lower byte
    proc->memory[newAddr + 1] = static_cast<char>((value >> 8) & 0xFF); // upper byte
}


