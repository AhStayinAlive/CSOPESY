// ForInstruction.cpp
#include "ForInstruction.h"
#include "utils.h"
#include "process.h"
ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions)
    : iterations(count), subInstructions(instructions) {
}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    logToFile(proc->name, "FOR loop starting (" + std::to_string(iterations) + " iterations)", coreId);

    for (int i = 0; i < iterations; ++i) {
        for (auto& instruction : subInstructions) {
            instruction->execute(proc, coreId);
        }
    }

    logToFile(proc->name, "FOR loop completed", coreId);
}