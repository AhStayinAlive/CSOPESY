#include "CLIManager.h"
#include "scheduler.h"
#include "ProcessManager.h"
#include "config.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

CLIManager::CLIManager() : generating(false) {}

void CLIManager::run() {
    std::string input;
    std::cout << "CSOPESY Emulator CLI\nType 'help' for commands.\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;
        handleCommand(input);
    }
}

void CLIManager::handleCommand(const std::string& input) {
    auto tokens = tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "scheduler-start") {
        if (generating) {
            std::cout << "Scheduler already running.\n";
            return;
        }

        const auto& config = Config::getInstance();
        startScheduler(config);
        generating = true;

        // Start generator thread
        schedulerThread = std::thread([this]() {
            const auto& config = Config::getInstance();
            while (generating) {
                auto proc = ProcessManager::createUniqueNamedProcess(config.minInstructions, config.maxInstructions, config.memPerProc);
                ProcessManager::addProcess(proc);
                addProcess(proc); // Add to scheduler queue
                /*std::cout << "[INFO] Created process: " << proc->name
                    << " (PID: " << proc->pid << ", Total Instructions: "
                    << proc->totalInstructions << ")\n";*/
                std::this_thread::sleep_for(std::chrono::seconds(config.batchProcessFreq));
            }
            });

        std::cout << "Scheduler started. Generating processes...\n";
    }
    else if (cmd == "scheduler-stop") {
        if (!generating) {
            std::cout << "Scheduler is not running.\n";
            return;
        }

        generating = false;
        if (schedulerThread.joinable()) schedulerThread.join();
        stopScheduler(); // graceful shutdown
        std::cout << "Scheduler stopped.\n";
    }
    else if (cmd == "screen" && tokens.size() > 1 && tokens[1] == "-ls") {
        showProcessList();
    }
    else if (cmd == "exit") {
        generating = false;
        if (schedulerThread.joinable()) schedulerThread.join();
        stopScheduler();
        std::cout << "Exiting CLI.\n";
        exit(0);
    }
    else {
        std::cout << "Unknown command: " << cmd << "\n";
    }
}

std::vector<std::string> CLIManager::tokenize(const std::string& input) const {
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

void CLIManager::showProcessList() const {
    auto running = ProcessManager::getRunningProcesses();
    auto waiting = ProcessManager::getWaitingProcesses();
    auto finished = ProcessManager::getFinishedProcesses();

    std::cout << "\n=== PROCESS LIST ===\n";
    std::cout << "RUNNING:\n";
    for (const auto& p : running) {
        std::cout << "  " << p->name << " (PID: " << p->pid
            << ") [" << *p->completedInstructions << "/" << p->totalInstructions << "]\n";
    }
    std::cout << "\nWAITING:\n";
    for (const auto& p : waiting) {
        std::cout << "  " << p->name << " (PID: " << p->pid
            << ") [" << *p->completedInstructions << "/" << p->totalInstructions << "]\n";
    }
    std::cout << "\nFINISHED:\n";
    for (const auto& p : finished) {
        std::cout << "  " << p->name << " (PID: " << p->pid
            << ") [" << p->totalInstructions << "/" << p->totalInstructions << "]\n";
    }
    std::cout << "=====================\n\n";
}
