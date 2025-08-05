#include "UserPrintInstruction.h"
#include "MemoryManager.h"
#include "process.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <cstdint>

UserPrintInstruction::UserPrintInstruction(const std::string& msg, const std::string& logPrefix)
    : message(msg), logPrefix(logPrefix) {
}

void UserPrintInstruction::execute(std::shared_ptr<Process> proc, int coreId) {
    std::string output = message;

    size_t pos = 0;
    while ((pos = output.find("\\\"", pos)) != std::string::npos) {
        output.replace(pos, 2, "\"");
    }
    while ((pos = output.find("\\\\", pos)) != std::string::npos) {
        output.replace(pos, 2, "\\");
    }

    // Trim whitespace from message
    output.erase(0, output.find_first_not_of(" \t"));
    output.erase(output.find_last_not_of(" \t") + 1);

    // Remove quotes if present
    while (!output.empty() && (output.front() == '"' || output.front() == '\'')) {
        output = output.substr(1);
    }
    while (!output.empty() && (output.back() == '"' || output.back() == '\'')) {
        output = output.substr(0, output.length() - 1);
    }

    // Check if this is a variable name (no spaces, no special chars except +)
    size_t plusPos = output.find(" + ");

    if (plusPos != std::string::npos) {
        // Handle "text" + variable pattern
        std::string textPart = output.substr(0, plusPos);
        std::string varPart = output.substr(plusPos + 3);

        // Remove any quotes from textPart
        while (!textPart.empty() && (textPart.front() == '"' || textPart.front() == '\'')) {
            textPart = textPart.substr(1);
        }
        while (!textPart.empty() && (textPart.back() == '"' || textPart.back() == '\'')) {
            textPart = textPart.substr(0, textPart.length() - 1);
        }

        // Trim whitespace from variable name
        varPart.erase(0, varPart.find_first_not_of(" \t"));
        varPart.erase(varPart.find_last_not_of(" \t") + 1);

        if (proc->variableTable.count(varPart)) {
            int varAddr = proc->variableTable[varPart];
            uint8_t lowByte = MemoryManager::getInstance().read(proc, varAddr);
            uint8_t highByte = 0;
            if (varAddr + 1 < proc->virtualMemoryLimit) {
                highByte = MemoryManager::getInstance().read(proc, varAddr + 1);
            }
            uint16_t varValue = lowByte | (static_cast<uint16_t>(highByte) << 8);
            output = textPart + std::to_string(varValue);
        }
        else {
            output = textPart + "[VAR_NOT_FOUND: " + varPart + "]";
        }
    }
    else if (proc->variableTable.count(output)) {
        // This is a single variable name - print its value
        int varAddr = proc->variableTable[output];
        uint8_t lowByte = MemoryManager::getInstance().read(proc, varAddr);
        uint8_t highByte = 0;
        if (varAddr + 1 < proc->virtualMemoryLimit) {
            highByte = MemoryManager::getInstance().read(proc, varAddr + 1);
        }
        uint16_t varValue = lowByte | (static_cast<uint16_t>(highByte) << 8);
        output = std::to_string(varValue);
    }
    // else: treat as literal text

    std::ostringstream logEntry;
    if (!logPrefix.empty()) {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | " << logPrefix
            << " | PRINT: \"" << output << "\"";
    }
    else {
        logEntry << "[" << getCurrentTimestamp() << "] "
            << "Core " << coreId
            << " | PID " << proc->pid
            << " | PRINT: \"" << output << "\"";
    }

    proc->logs.push_back(logEntry.str());
    logToFile(proc->name, logEntry.str(), coreId);
}