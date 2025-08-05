#include "ConsoleView.h"
#include "config.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// ANSI Color Codes
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"

// Text Colors
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BRIGHT_BLACK "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

#define CREAM "\033[38;5;230m"

void ConsoleView::clearScreen() {
#ifdef _WIN32
    system("CLS");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void ConsoleView::displayColoredLogs(const std::shared_ptr<Process>& proc, bool showAll) {
    if (proc->logs.empty()) {
        std::cout << BRIGHT_BLACK << "  [No logs available]\n" << RESET;
        return;
    }

    // Show last 15 logs for better readability in main view, or all logs in process-smi
    int logSize = static_cast<int>(proc->logs.size());
    int startIdx = (!showAll && logSize > 15) ? (logSize - 15) : 0;

    for (int i = startIdx; i < logSize; ++i) {
        std::string log = proc->logs[i];

        if (log.find("DECLARE") != std::string::npos) {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::string rest = log.substr(timestampEnd + 2);

                size_t declarePos = rest.find("DECLARE");
                if (declarePos != std::string::npos) {
                    std::cout << " " << rest.substr(0, declarePos);
                    std::cout << BRIGHT_BLUE << BOLD << "DECLARE" << RESET;

                    std::string afterDeclare = rest.substr(declarePos + 7);
                    size_t equalPos = afterDeclare.find(" = ");
                    if (equalPos != std::string::npos) {
                        std::cout << BRIGHT_YELLOW << afterDeclare.substr(0, equalPos) << RESET;
                        std::cout << " = " << BRIGHT_WHITE << afterDeclare.substr(equalPos + 3) << RESET;
                    }
                    else {
                        std::cout << afterDeclare;
                    }
                }
            }
            else {
                std::cout << log;
            }
        }
        else if (log.find("ADD") != std::string::npos || log.find("SUBTRACT") != std::string::npos) {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::string rest = log.substr(timestampEnd + 2);

                size_t opPos = rest.find("ADD");
                if (opPos == std::string::npos) opPos = rest.find("SUBTRACT");

                if (opPos != std::string::npos) {
                    std::cout << " " << rest.substr(0, opPos);
                    size_t opEnd = rest.find(":", opPos);
                    if (opEnd != std::string::npos) {
                        std::cout << BRIGHT_BLUE << BOLD << rest.substr(opPos, opEnd - opPos) << RESET;
                        std::cout << ": " << BRIGHT_WHITE << rest.substr(opEnd + 2) << RESET;
                    }
                }
            }
            else {
                std::cout << log;
            }
        }
        else if (log.find("PRINT") != std::string::npos) {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::string rest = log.substr(timestampEnd + 2);

                size_t printPos = rest.find("PRINT");
                if (printPos != std::string::npos) {
                    std::cout << " " << rest.substr(0, printPos);
                    std::cout << BRIGHT_BLUE << BOLD << "PRINT" << RESET;
                    std::cout << BRIGHT_WHITE << rest.substr(printPos + 5) << RESET;
                }
            }
            else {
                std::cout << log;
            }
        }
        else if (log.find("SLEEP") != std::string::npos) {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::string rest = log.substr(timestampEnd + 2);

                size_t sleepPos = rest.find("SLEEP");
                if (sleepPos != std::string::npos) {
                    std::cout << " " << rest.substr(0, sleepPos);
                    std::cout << BRIGHT_BLUE << BOLD << "SLEEP" << RESET;
                    std::cout << BRIGHT_WHITE << rest.substr(sleepPos + 5) << RESET;
                }
            }
            else {
                std::cout << log;
            }
        }
        else if (log.find("FOR") != std::string::npos) {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::string rest = log.substr(timestampEnd + 2);

                size_t forPos = rest.find("FOR");
                if (forPos != std::string::npos) {
                    std::cout << " " << rest.substr(0, forPos);
                    std::cout << BRIGHT_MAGENTA << BOLD << "FOR" << RESET;
                    std::cout << rest.substr(forPos + 3);
                }
            }
            else {
                std::cout << log;
            }
        }
        else {
            size_t timestampEnd = log.find("] ");
            if (timestampEnd != std::string::npos) {
                std::cout << BRIGHT_CYAN << log.substr(0, timestampEnd + 1) << RESET;
                std::cout << " " << BRIGHT_WHITE << log.substr(timestampEnd + 2) << RESET;
            }
            else {
                std::cout << BRIGHT_WHITE << log << RESET;
            }
        }
        std::cout << "\n";
    }

    if (!showAll && proc->logs.size() > 15) {
        std::stringstream logCountStr;
        logCountStr << proc->logs.size();
        std::cout << "\n" << BRIGHT_BLACK << "  ... showing last 15 logs (total: " << logCountStr.str() << ")" << RESET << "\n";
    }
}

void ConsoleView::show(const std::shared_ptr<Process>& proc) {
    std::string input;

    while (true) {
        clearScreen();
        displayProcessScreen(proc);

        std::cout << "\n" << CREAM << BOLD << proc->name << RESET << BRIGHT_WHITE << ":\\> " << RESET;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }
        else if (input == "process-smi") {
            clearScreen();

            std::cout << BRIGHT_CYAN << BOLD << "Process Details\n" << RESET;
            std::cout << BRIGHT_CYAN << "======================================\n" << RESET;
            std::cout << BRIGHT_YELLOW << "Process name: " << BRIGHT_WHITE << proc->name << std::endl;
            std::cout << BRIGHT_YELLOW << "ID: " << BRIGHT_WHITE << proc->pid << std::endl;

            int pageSizeKB = Config::getInstance().memPerFrame;
            int procMemKB = Config::getInstance().maxMemPerProc;
            int totalPages = procMemKB / pageSizeKB;

            int pagesInMemory = 0;
            int pagesInBacking = 0;

            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) pagesInMemory++;
                else pagesInBacking++;
            }

            std::cout << BRIGHT_CYAN << "----------------------------------------\n";
            std::cout << BRIGHT_GREEN << BOLD << "Memory Summary:\n" << RESET;
            std::cout << BRIGHT_CYAN << "  Page Size           : " << BRIGHT_WHITE << pageSizeKB << " KB\n";
            std::cout << BRIGHT_CYAN << "  Total Virtual Pages : " << BRIGHT_WHITE << totalPages << "\n";
            std::cout << BRIGHT_CYAN << "  Pages in Memory     : " << BRIGHT_GREEN << pagesInMemory << "\n";
            std::cout << BRIGHT_CYAN << "  Pages in Backing    : " << BRIGHT_YELLOW << pagesInBacking << "\n";
            std::cout << BRIGHT_CYAN << "  Approx. Used Memory : " << BRIGHT_WHITE << (pagesInMemory * pageSizeKB) << " KB\n";
            std::cout << BRIGHT_CYAN << "----------------------------------------\n" << RESET;

            std::cout << BRIGHT_MAGENTA << BOLD << "Logs:\n" << RESET;
            ConsoleView::displayColoredLogs(proc, true); // Show all logs in process-smi

            std::cout << BRIGHT_YELLOW << "\nCurrent instruction line: " << BRIGHT_WHITE << (proc->isFinished ? static_cast<int>(proc->instructions.size()) : proc->instructionPointer + 1) << std::endl;
            std::cout << BRIGHT_YELLOW << "Lines of code: " << BRIGHT_WHITE << proc->instructions.size() << std::endl;

            if (proc->isFinished) {
                std::cout << BRIGHT_GREEN << BOLD << "\nFinished!" << std::endl << RESET;
            }
        }
        else if (!input.empty()) {
            std::cout << BRIGHT_RED << "Unknown command: " << BRIGHT_YELLOW << input << BRIGHT_RED << ". Available commands: " << BRIGHT_CYAN << "process-smi" << BRIGHT_RED << ", " << BRIGHT_CYAN << "exit\n" << RESET;
            std::cout << BRIGHT_WHITE << "Press [Enter] to continue..." << RESET;
            std::cin.get();
        }
    }
}

void ConsoleView::displayProcessScreen(const std::shared_ptr<Process>& proc) {
    std::cout << BRIGHT_CYAN << BOLD << "+--------------------------------------+\n";
    std::cout << "|         PROCESS CONSOLE VIEW         |\n";
    std::cout << "+--------------------------------------+\n" << RESET;
    std::cout << BRIGHT_YELLOW << "| Process Name : " << BRIGHT_WHITE << proc->name << "\n";
    std::cout << BRIGHT_YELLOW << "| PID          : " << BRIGHT_WHITE << proc->pid << "\n";
    std::cout << BRIGHT_YELLOW << "| Core Assigned: " << BRIGHT_WHITE << proc->coreAssigned << "\n";
    std::cout << BRIGHT_YELLOW << "| Start Time   : " << BRIGHT_WHITE << proc->startTime << "\n";
    std::cout << BRIGHT_YELLOW << "| End Time     : " << BRIGHT_WHITE << (proc->isFinished ? proc->endTime : "N/A") << "\n";
    std::cout << BRIGHT_YELLOW << "| Instructions : " << BRIGHT_WHITE << *proc->completedInstructions << " / " << proc->instructions.size() << "\n";
    std::cout << BRIGHT_CYAN << BOLD << "+--------------------------------------+\n" << RESET;

    std::cout << BRIGHT_MAGENTA << BOLD << "\nLogs:\n" << RESET;
    ConsoleView::displayColoredLogs(proc, false); // Show last 15 logs in main view

    std::cout << BRIGHT_YELLOW << "\nCurrent instruction line: " << BRIGHT_WHITE << (proc->isFinished ? static_cast<int>(proc->instructions.size()) : proc->instructionPointer + 1) << "\n";
    std::cout << BRIGHT_YELLOW << "Lines of code: " << BRIGHT_WHITE << proc->instructions.size() << "\n";

    std::cout << BRIGHT_CYAN << BOLD << "\nStatus: " << RESET;
    if (proc->isFinished) {
        if (proc->hasMemoryViolation) {
            std::cout << BRIGHT_RED << BOLD << "[ERROR] " << BRIGHT_WHITE << "Memory access violation at address " << BRIGHT_YELLOW << proc->memoryViolationAddress << BRIGHT_WHITE << ".\n";
            std::cout << BRIGHT_RED << "Process terminated due to invalid memory access.\n" << RESET;
        }
        else if (*proc->completedInstructions == static_cast<int>(proc->instructions.size())) {
            std::cout << BRIGHT_GREEN << BOLD << "[OK] " << BRIGHT_WHITE << "Finished successfully.\n";
            std::cout << BRIGHT_GREEN << "Finished!\n" << RESET;
        }
        else {
            std::cout << BRIGHT_RED << BOLD << "[ERROR] " << BRIGHT_WHITE << "Terminated early due to error at instruction "
                << proc->instructionPointer + 1 << ".\n" << RESET;
        }
    }
    else if (proc->isRunning) {
        std::cout << BRIGHT_GREEN << BOLD << "[RUNNING] " << BRIGHT_WHITE << "Currently running.\n" << RESET;
    }
    else {
        std::cout << BRIGHT_YELLOW << BOLD << "[WAITING] " << BRIGHT_WHITE << "Queued or waiting.\n" << RESET;
    }
    std::cout << BRIGHT_CYAN << "\nAvailable commands: " << BRIGHT_YELLOW << "process-smi" << BRIGHT_CYAN << ", " << BRIGHT_YELLOW << "exit" << RESET;

}
