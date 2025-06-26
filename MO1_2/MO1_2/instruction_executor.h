// instruction_executor.cpp
#include "process.h"
#include "Instruction.h"
#include <stdexcept>
#include <iostream>

bool executeSingleInstruction(std::shared_ptr<Process> proc,
    std::shared_ptr<Instruction> instruction,
    int coreId) {
    try {
        // Execute the instruction
        instruction->execute(proc, coreId);

        // Increment completed instructions counter
        (*proc->completedInstructions)++;

        return true;
    }
    catch (const std::exception& e) {
        // Log the error
        std::string errorMsg = "Error executing instruction: " + std::string(e.what());
        proc->logs.push_back(errorMsg);

        // Mark process as finished due to error
        proc->isFinished = true;
        proc->isRunning = false;

        std::cerr << "Process " << proc->name << " encountered error: " << e.what() << std::endl;

        return false;
    }
    catch (...) {
        // Handle any other exceptions
        std::string errorMsg = "Unknown error executing instruction";
        proc->logs.push_back(errorMsg);

        proc->isFinished = true;
        proc->isRunning = false;

        std::cerr << "Process " << proc->name << " encountered unknown error" << std::endl;

        return false;
    }
}