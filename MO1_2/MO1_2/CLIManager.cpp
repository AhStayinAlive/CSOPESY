#include "CLIManager.h"
#include "config.h"
#include "ProcessManager.h"
#include "ConsoleView.h"
#include "scheduler.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <sstream>

CLIManager::CLIManager() : schedulerThread(), generating(false) {}

void printBanner() {
    std::cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    std::cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    std::cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    std::cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    std::cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    std::cout << "-------------------------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n";
    std::cout << "Developers: \nDel Gallego, Neil Patrick\n\n";
    std::cout << "Last updated: 01-18-2024\n";
    std::cout << "-------------------------------------------------------------\n";
}

void CLIManager::run() {
    printBanner();
    std::string input;
    std::cout << "Welcome to the CLI Scheduler. Type 'help' for commands.\n";
    while (true) {
        std::cout << "root:\\> ";
        std::getline(std::cin, input);
        if (input == "exit") {
            stopScheduler();
            break;
        }
        handleCommand(input);
    }
}

void CLIManager::handleCommand(const std::string& input) {
    auto tokens = tokenize(input);
    if (tokens.empty()) return;

    const std::string& cmd = tokens[0];

    if (cmd == "initialize") {
        if (Config::getInstance().loadFromFile("config.txt")) {
            std::cout << "Configuration loaded.\n";
            startScheduler(Config::getInstance());
        }
        else {
            std::cout << "Failed to load config.txt.\n";
        }
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-s") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            proc = ProcessManager::createNamedProcess(name);
            ProcessManager::addProcess(proc);
            std::cout << "Created and queued process " << name << "\n";
        }
        ConsoleView::show(proc);
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);
        if (proc) {
            ConsoleView::show(proc);
        }
        else {
            std::cout << "Process " << name << " not found.\n";
        }
    }

    else if (cmd == "screen" && tokens.size() == 2 && tokens[1] == "-ls") {
        showProcessList();
    }

    else if (cmd == "scheduler-start") {
        if (!generating) {
            generating = true;
            schedulerThread = std::thread([&] {
                try {
                    auto& config = Config::getInstance();
                    while (generating) {
                        auto proc = ProcessManager::createUniqueNamedProcess(config.minInstructions, config.maxInstructions);
                        ProcessManager::addProcess(proc);
                        std::this_thread::sleep_for(std::chrono::seconds(config.batchProcessFreq));
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "[Scheduler Error] " << e.what() << "\n";
                }
                });
            std::cout << "Batch process generation started.\n";
        }
        else {
            std::cout << "Already generating processes.\n";
        }
    }


    else if (cmd == "scheduler-stop") {
        if (generating) {
            generating = false;
            if (schedulerThread.joinable()) schedulerThread.join();
            std::cout << "Stopped generating batch processes.\n";
        }
        else {
            std::cout << "Scheduler was not running.\n";
        }
    }

    else if (cmd == "report") {
        generateReport();
    }

    else if (cmd == "help") {
        showHelp();
    }

    else {
        std::cout << "Unknown command. Type 'help' for available commands.\n";
    }
}

void CLIManager::stopScheduler() {
    generating = false;
    if (schedulerThread.joinable()) schedulerThread.join();
}

void CLIManager::showHelp() const {
    std::cout << "Available commands:\n"
        << "  initialize         - Load config.txt and start scheduler\n"
        << "  screen -s [name]   - Create or open a process\n"
        << "  screen -r [name]   - Resume and inspect a process\n"
        << "  screen -ls         - Show running and finished processes\n"
        << "  scheduler-start    - Begin periodic batch process generation\n"
        << "  scheduler-stop     - Stop batch process generation\n"
        << "  report             - Generate utilization report\n"
        << "  exit               - Exit the CLI\n";
}

std::vector<std::string> CLIManager::tokenize(const std::string& input) const {
    std::stringstream ss(input);
    std::string word;
    std::vector<std::string> tokens;
    while (ss >> word) tokens.push_back(word);
    return tokens;
}

void CLIManager::showProcessList() const {
    const auto& all = ProcessManager::getAllProcesses();
    int running = 0;
    for (auto& p : all) {
        if (p->isRunning && !p->isFinished) running++;
    }

    std::cout << "CPU utilization: " << (running * 100 / Config::getInstance().numCPU) << "%\n";
    std::cout << "Cores used: " << running << "\n";
    std::cout << "Cores available: " << Config::getInstance().numCPU - running << "\n\n";

    std::cout << "Running processes:\n";
    for (auto& p : all) {
        if (p->isRunning && !p->isFinished) {
            std::cout << p->name << " | Core " << p->coreAssigned
                << " | " << *(p->completedInstructions)
                << "/" << p->instructions.size() << "\n";
        }
    }

    std::cout << "\nFinished processes:\n";
    for (auto& p : all) {
        if (p->isFinished) {
            std::cout << p->name << " | Ended: " << p->endTime << "\n";
        }
    }
}
