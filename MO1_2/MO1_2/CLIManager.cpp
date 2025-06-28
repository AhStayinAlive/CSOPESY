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
#include <iomanip>

CLIManager::CLIManager() : schedulerThread(), generating(false) {}

void printBanner() {
    std::cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    std::cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    std::cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    std::cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    std::cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    std::cout << "-------------------------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n";
    std::cout << "\nDevelopers:\n";
    std::cout << "Hanna Angela D. De Los Santos\n";
    std::cout << "Joseph Dean T. Enriquez\n";
    std::cout << "Shanette Giane G. Presas\n";
    std::cout << "Jersey K. To\n";
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
    static bool initialized = false;

    auto tokens = tokenize(input);
    if (tokens.empty()) return;

    const std::string& cmd = tokens[0];

    if (cmd != "initialize" && !initialized) {
        std::cout << "Please run 'initialize' first.\n";
        return;
    }

    if (cmd == "initialize") {
        if (Config::getInstance().loadFromFile("config.txt")) {
            std::cout << "Configuration loaded.\n";

            auto& config = Config::getInstance();
            if (config.scheduler == "FCFS" || config.scheduler == "fcfs") {
                schedulerType = SchedulerType::FCFS;
            }
            else if (config.scheduler == "RR" || config.scheduler == "rr") {
                schedulerType = SchedulerType::ROUND_ROBIN;
            }
            else {
                std::cerr << "Unknown scheduler type in config: " << config.scheduler << ", defaulting to FCFS.\n";
                schedulerType = SchedulerType::FCFS;
            }

            initialized = true;
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
            addProcess(proc);
            std::cout << "Created and queued process " << name << "\n";
            ConsoleView::show(proc);
        }

        else {
            std::cout << name << " already exists." << "\n";
        }
        
        
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);
        if (proc && !proc->isFinished) {
            ConsoleView::show(proc);
        }
        else {
            std::cout << "Process " << name << " not found or already finished.\n";
        }
    }

    else if (cmd == "screen" && tokens.size() == 2 && tokens[1] == "-ls") {
        showProcessList();
    }

    else if (cmd == "process-smi" && tokens.size() == 2) {
        std::string name = tokens[1];
        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            std::cout << "Process '" << name << "' not found.\n";
            return;
        }
        std::cout << "---- Logs for process: " << name << " ----\n";
        for (const auto& log : proc->logs) {
            std::cout << log << "\n";
        }
        std::cout << "----------------------------------------\n";
    }

    else if (cmd == "scheduler-start" || cmd == "scheduler-test") {
        if (!generating) {
            generating = true;
            startScheduler(Config::getInstance());
            schedulerThread = std::thread([&] {
                try {
                    auto& config = Config::getInstance();
                    while (generating) {
                        auto proc = ProcessManager::createUniqueNamedProcess(config.minInstructions, config.maxInstructions);
                        ProcessManager::addProcess(proc);
                        addProcess(proc);
                        std::this_thread::sleep_for(std::chrono::seconds(config.batchProcessFreq));
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "[Scheduler Error] " << e.what() << "\n";
                }
                });
            std::cout << "Batch process generation started.\n";
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

    else if (cmd == "report-util") {
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
    ::stopScheduler();
}

void CLIManager::showHelp() const {
    std::cout << "Available commands:\n"
        << "  initialize         - Load config.txt and prepare scheduler\n"
        << "  screen -s [name]   - Create or open a process\n"
        << "  process-smi [name] - Show logs for a specific process\n"
        << "  screen -r [name]   - Resume and inspect a process\n"
        << "  screen -ls         - Show running and finished processes\n"
        << "  scheduler-start    - Begin periodic batch process generation\n"
        << "  scheduler-stop     - Stop batch process generation\n"
        << "  report-util        - Generate utilization report\n"
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
    int running = 0, finished = 0, waiting = 0;

    for (const auto& p : all) {
        if (p->isRunning && !p->isFinished) running++;
        else if (p->isFinished) finished++;
        else waiting++;
    }

    int numCPU = Config::getInstance().numCPU;
    if (numCPU == 0) {
        std::cout << "Error: numCPU is 0. Please check your config.txt file.\n";
        return;
    }

    double utilization = (static_cast<double>(running) / numCPU) * 100.0;
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "CPU utilization: " << std::min(utilization, 100.0) << "%\n";
    std::cout << "Cores used: " << running << "\n";
    std::cout << "Cores available: " << std::max(0, numCPU - running) << "\n";
    std::cout << "Total processes: " << all.size() << "\n";
    std::cout << "Running: " << running << " | Waiting: " << waiting << " | Finished: " << finished << "\n\n";

    std::cout << "Running processes:\n";
    for (const auto& p : all) {
        if (p->isRunning && !p->isFinished) {
            std::cout << "  " << p->name << " (PID:" << p->pid << ") | Core " << p->coreAssigned
                << " | " << *p->completedInstructions << "/" << p->instructions.size()
                << " | Started: " << p->startTime << "\n";
        }
    }

    std::cout << "\nWaiting processes:\n";
    for (const auto& p : all) {
        if (!p->isRunning && !p->isFinished) {
            std::cout << "  " << p->name << " (PID:" << p->pid << ") | "
                << *p->completedInstructions << "/" << p->instructions.size() << " completed\n";
        }
    }

    std::cout << "\nFinished processes:\n";
    for (const auto& p : all) {
        if (p->isFinished) {
            std::cout << "  " << p->name << " (PID:" << p->pid << ") | Finished: " << p->endTime
                << " | " << *p->completedInstructions << "/" << p->instructions.size() << " completed\n";
        }
    }
}
