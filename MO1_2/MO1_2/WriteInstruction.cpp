#include "WriteInstruction.h"
#include "MemoryManager.h"
#include "utils.h"
#include "Process.h"
#include <stdexcept>
#include <string>

WriteInstruction::WriteInstruction(size_t addr, uint16_t val)
    : address(addr), value(val) {
}

void WriteInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Validate address range
    if (address >= proc->getRequiredMemory()) {
        throw std::runtime_error("Access violation at 0x" + to_hex(address));
    }

    // Ensure page is loaded
    size_t page = address / MemoryManager::getInstance().getFrameSize();
    if (!proc->loadedPages.count(page)) {
        MemoryManager::getInstance().handlePageFault(proc, page);
    }

    // Write the value to memory
    proc->memory[address] = value;
}