#pragma once
#ifndef INSTRUCTION_EXECUTOR_H
#define INSTRUCTION_EXECUTOR_H

#include "process.h"
#include "utils.h"
#include <thread>
#include <chrono>
#include <cctype>
#include <algorithm>
#include <map>              // ✅ Needed for std::map
#include <unordered_map>    // ✅ Needed for std::unordered_map
#include <string>

// --- Check if a string is numeric ---
inline bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// --- Safely parse a numeric string or throw with logging ---
inline int safeParse(const std::string& s, const std::string& context, const std::string& processName = "", int coreId = -1) {
    if (!isNumber(s)) {
        logToFile(processName, "Invalid numeric input in " + context + ": \"" + s + "\"", coreId);
        throw std::invalid_argument("Non-numeric argument in " + context + ": \"" + s + "\"");
    }
    return std::stoi(s);
}

// --- Resolve variable or raw number ---
inline uint16_t resolve(const std::map<std::string, uint16_t>& mem, const std::string& s) {
    if (mem.count(s)) return mem.at(s);
    if (isNumber(s)) return static_cast<uint16_t>(std::stoi(s));
    throw std::invalid_argument("Failed to resolve: " + s);
}

// --- Main executor ---
inline bool executeSingleInstruction(std::shared_ptr<Process>& proc, const Instruction& ins, int coreId = -1) {
    try {
        auto& mem = proc->memory;

        if (ins.opcode == "PRINT") {
            std::string msg = ins.arg1;
            if (mem.count(ins.arg1)) {
                msg += " = " + std::to_string(mem[ins.arg1]);
            }
            logToFile(proc->name, msg, coreId);
            proc->logs.push_back(msg);
        }
        else if (ins.opcode == "DECLARE") {
            mem[ins.arg1] = static_cast<uint16_t>(std::stoi(ins.arg2));
            logToFile(proc->name, "DECLARE " + ins.arg1 + " = " + ins.arg2, coreId);
        }
        else if (ins.opcode == "ADD") {
            uint16_t lhs = resolve(mem, ins.arg2);
            uint16_t rhs = resolve(mem, ins.arg3);
            mem[ins.arg1] = lhs + rhs;
            logToFile(proc->name, "ADD " + ins.arg1 + " = " + std::to_string(lhs) + " + " + std::to_string(rhs), coreId);
        }
        else if (ins.opcode == "SUBTRACT") {
            uint16_t lhs = resolve(mem, ins.arg2);
            uint16_t rhs = resolve(mem, ins.arg3);
            mem[ins.arg1] = lhs - rhs;
            logToFile(proc->name, "SUBTRACT " + ins.arg1 + " = " + std::to_string(lhs) + " - " + std::to_string(rhs), coreId);
        }
        else if (ins.opcode == "SLEEP") {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(ins.arg1)));
        }
        else if (ins.opcode == "FOR") {
            for (int i = 0; i < ins.loopCount; ++i) {
                for (const auto& sub : ins.subInstructions) {
                    if (!executeSingleInstruction(proc, sub, coreId)) return false;
                }
            }
        }

        (*proc->completedInstructions)++;
        if (*proc->completedInstructions > proc->instructions.size()) {
            *proc->completedInstructions = proc->instructions.size();
        }
        return true;

    }
    catch (const std::exception& e) {
        std::ostringstream errMsg;
        errMsg << "[Error] Failed at instruction " << proc->instructionPointer
            << " (" << ins.opcode << "): " << e.what();
        logToFile(proc->name, errMsg.str(), coreId);
        proc->logs.push_back(errMsg.str());
        return false;
    }
}

#endif // INSTRUCTION_EXECUTOR_H
