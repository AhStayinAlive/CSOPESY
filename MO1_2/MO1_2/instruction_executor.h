#include "process.h"
#include "Instruction.h"
#include <stdexcept>
#include <iostream>

bool executeSingleInstruction(std::shared_ptr<Process> proc,
    std::shared_ptr<Instruction> instruction,
    int coreId) {
    try {
        instruction->execute(proc, coreId);

        (*proc->completedInstructions)++;

        return true;
    }
    catch (const std::exception& e) {
        std::string errorMsg = "Error executing instruction: " + std::string(e.what());
        proc->logs.push_back(errorMsg);

        proc->isFinished = true;
        proc->isRunning = false;

        std::cerr << "Process " << proc->name << " encountered error: " << e.what() << std::endl;

        return false;
    }
    catch (...) {
        std::string errorMsg = "Unknown error executing instruction";
        proc->logs.push_back(errorMsg);

        proc->isFinished = true;
        proc->isRunning = false;

        std::cerr << "Process " << proc->name << " encountered unknown error" << std::endl;

        return false;
    }
}
