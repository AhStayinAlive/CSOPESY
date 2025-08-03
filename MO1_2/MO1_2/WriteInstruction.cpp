#include "WriteInstruction.h"
#include "MemoryManager.h"
#include "utils.h"
#include "Process.h"
#include <stdexcept>
#include <string>

WriteInstruction::WriteInstruction(size_t addr, uint16_t val)
    : address(addr), value(val) {
}

void MemoryManager::write(std::shared_ptr<Process> proc, size_t address, uint16_t value) {
    size_t page = address / frameSize;

    // Demand paging: load if not present
    if (!proc->loadedPages.count(page)) {
        handlePageFault(proc, page);
    }

    // Bounds check
    if (address + 1 >= proc->memory.size()) {
        throw std::runtime_error("Write out of bounds at " + std::to_string(address));
    }

    // Store value as two bytes (little-endian)
    proc->memory[address] = value & 0xFF;
    proc->memory[address + 1] = (value >> 8) & 0xFF;
}