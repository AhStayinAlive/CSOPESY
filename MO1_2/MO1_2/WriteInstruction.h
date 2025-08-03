#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include "Instruction.h"
#include "Process.h"
#include "MemoryManager.h"
#include "utils.h" // for to_hex

class WriteInstruction : public Instruction {
    size_t address;
    uint16_t value;
public:
    WriteInstruction(size_t address, uint16_t value);

    void execute(std::shared_ptr<Process> proc, int coreId) override;
};