#pragma once
#ifndef INSTRUCTION_EXECUTOR_H
#define INSTRUCTION_EXECUTOR_H

#include "instruction.h"
#include "process.h"
#include "utils.h"
#include <sstream>

// Just runs the instruction
inline bool executeSingleInstruction(std::shared_ptr<Process>& proc, std::shared_ptr<Instruction>& ins, int coreId = -1) {
    try {
        ins->execute(proc, coreId);
        (*proc->completedInstructions)++;
        if (*proc->completedInstructions > proc->instructions.size()) {
            *proc->completedInstructions = proc->instructions.size();
        }
        return true;
    }
    catch (const std::exception& e) {
        std::ostringstream errMsg;
        errMsg << "[Error] Failed at instruction " << proc->instructionPointer
            << ": " << e.what();
        logToFile(proc->name, errMsg.str(), coreId);
        proc->logs.push_back(errMsg.str());
        return false;
    }
}

#endif // INSTRUCTION_EXECUTOR_H
