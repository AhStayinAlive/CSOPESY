// ForInstruction.cpp
#include "ForInstruction.h"

ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& body)
    : loopCount(count), subInstructions(body) {
}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    for (int i = 0; i < loopCount; ++i) {
        for (auto& instr : subInstructions) {
            instr->execute(proc, coreId);
        }
    }
}