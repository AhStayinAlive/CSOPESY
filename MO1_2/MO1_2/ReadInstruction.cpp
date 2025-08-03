#include "ReadInstruction.h"
#include "MemoryManager.h"
#include "Process.h"
#include "utils.h"
#include <stdexcept>
#include <string>

ReadInstruction::ReadInstruction(const std::string& var, size_t addr)
    : varName(var), address(addr) {
}

void ReadInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    // Validate address range
    if (address >= proc->getRequiredMemory()) {
        throw std::runtime_error("Invalid address: 0x" + to_hex(address));
    }

    // Ensure page is loaded
    size_t page = address / MemoryManager::getInstance().getFrameSize();
    if (!proc->loadedPages.count(page)) {
        MemoryManager::getInstance().handlePageFault(proc, page);
    }

    // Read value (0 if uninitialized)
    uint16_t value = 0;
    if (address + 1 < proc->memory.size()) {
        value = (static_cast<uint16_t>(proc->memory[address + 1]) << 8) | static_cast<uint8_t>(proc->memory[address]);
    }
    proc->declareVariable(varName, value);

}