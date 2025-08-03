#pragma once
#include <string>
#include <memory>
#include <stdexcept>
#include "Instruction.h"          // Assuming Instruction is defined here
#include "Process.h"              // Assuming Process is defined here
#include "MemoryManager.h"        // For MemoryManager::getInstance()
#include "utils.h" 

class ReadInstruction : public Instruction {
    std::string varName;
    size_t address;
public:
    ReadInstruction(const std::string& var, size_t addr);

    void execute(std::shared_ptr<Process> proc, int coreId) override;
};