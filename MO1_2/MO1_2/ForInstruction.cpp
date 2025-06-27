
// ForInstruction.cpp
#include "ForInstruction.h"
#include "utils.h"
#include "process.h"

ForInstruction::ForInstruction(int count, const std::vector<std::shared_ptr<Instruction>>& instructions, int nesting)
    : iterations(count), subInstructions(instructions), nestingLevel(nesting) {
}

void ForInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string nestingStr = "";
    for (int i = 0; i < nestingLevel; i++) {
        nestingStr += "  "; // Indent based on nesting level
    }

    logToFile(proc->name, nestingStr + "FOR loop level " + std::to_string(nestingLevel) +
        " starting (" + std::to_string(iterations) + " iterations)", coreId);

    for (int i = 0; i < iterations; ++i) {
        for (auto& instruction : subInstructions) {
            instruction->execute(proc, coreId);
        }
    }

    logToFile(proc->name, nestingStr + "FOR loop level " + std::to_string(nestingLevel) + " completed", coreId);
}