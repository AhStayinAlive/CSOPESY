#include "ConsoleView.h"
#include "config.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void ConsoleView::clearScreen() {
#ifdef _WIN32
    system("CLS");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void ConsoleView::show(const std::shared_ptr<Process>& proc) {
    std::string input;

    while (true) {
        clearScreen();
        displayProcessScreen(proc);

        std::cout << "\n" << proc->name << ":\\> ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break; 
        }
        else if (input == "process-smi") {
            clearScreen();
            std::cout << "Process name: " << proc->name << std::endl;
            std::cout << "ID: " << proc->pid << std::endl;
            int pageSizeKB = Config::getInstance().memPerFrame;         // already in KB
            int procMemKB = Config::getInstance().maxMemPerProc;        // already in KB
            int totalPages = procMemKB / pageSizeKB;

            int pagesInMemory = 0;
            int pagesInBacking = 0;

            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) pagesInMemory++;
                else pagesInBacking++;
            }

            std::cout << "----------------------------------------\n";
            std::cout << "Memory Summary:\n";
            std::cout << "  Page Size           : " << pageSizeKB << " KB\n";
            std::cout << "  Total Virtual Pages : " << totalPages << "\n";
            std::cout << "  Pages in Memory     : " << pagesInMemory << "\n";
            std::cout << "  Pages in Backing    : " << pagesInBacking << "\n";
            std::cout << "  Approx. Used Memory : " << (pagesInMemory * pageSizeKB) << " KB\n";
            std::cout << "----------------------------------------\n";

            std::cout << "Logs:\n";
            if (proc->logs.empty()) {
                std::cout << "  [No logs available]\n";
            }
            else {
                for (const auto& log : proc->logs) {
                    std::cout << log << "\n";
                }
            }

            std::cout << "\nCurrent instruction line: " << (proc->isFinished ? static_cast<int>(proc->instructions.size()) : proc->instructionPointer + 1) << std::endl;
            std::cout << "Lines of code: " << proc->instructions.size() << std::endl;

            if (proc->isFinished) {
                std::cout << "\nFinished!" << std::endl;
            }
        }
        else if (!input.empty()) {
            std::cout << "Unknown command: " << input << ". Available commands: process-smi, exit\n";
            std::cout << "Press [Enter] to continue...";
            std::cin.get();
        }
    }
}

void ConsoleView::displayProcessScreen(const std::shared_ptr<Process>& proc) {
    std::cout << "+--------------------------------------+\n";
    std::cout << "|         PROCESS CONSOLE VIEW         |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "| Process Name : " << proc->name << "\n";
    std::cout << "| PID          : " << proc->pid << "\n";
    std::cout << "| Core Assigned: " << proc->coreAssigned << "\n";
    std::cout << "| Start Time   : " << proc->startTime << "\n";
    std::cout << "| End Time     : " << (proc->isFinished ? proc->endTime : "N/A") << "\n";
    std::cout << "| Instructions : " << *proc->completedInstructions << " / " << proc->instructions.size() << "\n";
    std::cout << "+--------------------------------------+\n";

    std::cout << "\nLogs:\n";
    if (proc->logs.empty()) {
        std::cout << "  [No logs available]\n";
    }
    else {
        for (const auto& log : proc->logs) {
            std::cout << log << "\n";
        }
    }

    std::cout << "\nCurrent instruction line: " << (proc->isFinished ? static_cast<int>(proc->instructions.size()) : proc->instructionPointer + 1) << "\n";
    std::cout << "Lines of code: " << proc->instructions.size() << "\n";

    std::cout << "\nStatus: ";
    if (proc->isFinished) {
        if (proc->hasMemoryViolation) {
            std::cout << "[ERROR] Memory access violation at address " << proc->memoryViolationAddress << ".\n";
            std::cout << "Process terminated due to invalid memory access.\n";
        }
        else if (*proc->completedInstructions == static_cast<int>(proc->instructions.size())) {
            std::cout << "[OK] Finished successfully.\n";
            std::cout << "Finished!\n";
        }
        else {
            std::cout << "[ERROR] Terminated early due to error at instruction "
                << proc->instructionPointer + 1 << ".\n";
        }
    }
    else if (proc->isRunning) {
        std::cout << "[RUNNING] Currently running.\n";
    }
    else {
        std::cout << "[WAITING] Queued or waiting.\n";
    }
    std::cout << "\nAvailable commands: process-smi, exit";
}