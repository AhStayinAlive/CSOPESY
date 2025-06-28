#include "ForInstruction.h"
#include "utils.h"
#include "process.h"

ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions, int nesting)
    : iterations(count), subInstructions(instructions), nestingLevel(nesting) {
}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string nestingStr = "";
    for (int i = 0; i < nestingLevel; i++) {
        nestingStr += "  ";
    }

    for (int i = 0; i < iterations; ++i) {
        for (auto& instruction : subInstructions) {
            instruction->execute(proc, proc->coreAssigned);
        }
    }
}
