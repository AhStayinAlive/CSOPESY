#include "ForInstruction.h"
#include "utils.h"
#include "process.h"
#include "MemoryManager.h"

ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions, int nesting)
    : iterations(count), subInstructions(instructions), nestingLevel(nesting) {
}

//void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
//    std::string nestingStr = "";
//    for (int i = 0; i < nestingLevel; i++) {
//        nestingStr += "  ";
//    }
//
//    for (int i = 0; i < iterations; ++i) {
//        for (auto& instruction : subInstructions) {
//            instruction->execute(proc, proc->coreAssigned);
//        }
//    }
//}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string nestingStr(nestingLevel * 2, ' ');

    for (int i = 0; i < iterations; ++i) {
        for (auto& instruction : subInstructions) {
            size_t virtualPage = i / MemoryManager::getInstance().getFrameSize();

            if (!proc->loadedPages.count(virtualPage)) {
                std::string var;
                int val;

                if (MemoryManager::getInstance().loadFromBackingStore(proc, virtualPage, var, val)) {
                    // Write val into memory at the address assigned to var
                    if (proc->variableAddressMap.count(var)) {
                        size_t addr = proc->variableAddressMap[var];
                        proc->memory[addr] = static_cast<char>(val & 0xFF);
                        proc->memory[addr + 1] = static_cast<char>((val >> 8) & 0xFF);
                    }
                }
                else {
                    // First time this page is being used — initialize it
                    if (proc->variableAddressMap.count(var)) {
                        size_t addr = proc->variableAddressMap[var];
                        proc->memory[addr] = static_cast<char>(val & 0xFF);
                        proc->memory[addr + 1] = static_cast<char>((val >> 8) & 0xFF);
                        MemoryManager::getInstance().writeToBackingStore(proc, virtualPage, var, val);
                    }
                }

                proc->loadedPages.insert(virtualPage); // Mark page as loaded
            }

            instruction->execute(proc, proc->coreAssigned);
        }
    }
}


