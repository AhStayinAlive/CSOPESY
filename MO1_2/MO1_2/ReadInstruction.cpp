#include "ReadInstruction.h"
#include "MemoryManager.h"
#include "Process.h"
#include "utils.h"
#include <stdexcept>
#include <string>

ReadInstruction::ReadInstruction(const std::string& var, size_t addr)
    : varName(var), address(addr) {
}

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