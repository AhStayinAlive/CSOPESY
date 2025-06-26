#include "ConsoleView.h"
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
    displayProcessScreen(proc);  // Wrapper function
}

void ConsoleView::displayProcessScreen(const std::shared_ptr<Process>& proc) {
    clearScreen();

    std::cout << "+--------------------------------------+\n";
    std::cout << "|         PROCESS CONSOLE VIEW        |\n";
    std::cout << "+--------------------------------------+\n";
    std::cout << "| Process Name : " << proc->name << "\n";
    std::cout << "| PID          : " << proc->pid << "\n";
    std::cout << "| Core Assigned: " << proc->coreAssigned << "\n";
    std::cout << "| Start Time   : " << proc->startTime << "\n";
    std::cout << "| End Time     : " << (proc->isFinished ? proc->endTime : "N/A") << "\n";
    std::cout << "| Instructions : " << *proc->completedInstructions << " / " << proc->instructions.size() << "\n";
    std::cout << "+--------------------------------------+\n";

    std::cout << "\nInstruction Logs:\n";
    if (proc->logs.empty()) {
        std::cout << "  [No logs available]\n";
    }
    else {
        int logNum = 1;
        for (const auto& log : proc->logs) {
            std::cout << "  [" << std::setw(2) << logNum++ << "] " << log << "\n";
        }
    }


    std::cout << "\nStatus: ";
    if (proc->isFinished) {
        if (*proc->completedInstructions == static_cast<int>(proc->instructions.size())) {
            std::cout << "[OK] Finished successfully.\n";
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

    std::cout << "\nPress [Enter] to exit this view...";
    std::cin.get();
}