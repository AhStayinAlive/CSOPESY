#include "CLIManager.h"
#include "scheduler.h"
#include "ProcessManager.h"
#include "MemoryManager.h"
#include "ConsoleView.h"
#include "config.h"
#include "utils.h"
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
    static bool initialized = false;

    auto tokens = tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];

    if (cmd == "initialize") {
        if (Config::getInstance().loadFromFile("config.txt")) {
            std::cout << "Configuration loaded.\n";

            auto& config = Config::getInstance();
            /*if (config.scheduler == "FCFS" || config.scheduler == "fcfs") {
                schedulerType = SchedulerType::FCFS;
            }
            else if (config.scheduler == "RR" || config.scheduler == "rr") {
                schedulerType = SchedulerType::ROUND_ROBIN;
            }
            else {
                std::cerr << "Unknown scheduler type in config: " << config.scheduler << ", defaulting to FCFS.\n";
                schedulerType = SchedulerType::FCFS;
            }*/

            initialized = true;
        }
        else {
            std::cout << "Failed to load config.txt.\n";
        }
    }
    else if (cmd == "scheduler-start") {
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
                auto proc = ProcessManager::createUniqueNamedProcess(config.minInstructions, config.maxInstructions, config.minMemPerProc);
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
    else if (cmd == "vmstat") {
        MemoryManager::getInstance().printMemoryStats(ProcessManager::getAllProcesses());
        //const auto& config = Config::getInstance();
        //auto& memMgr = MemoryManager::getInstance();

        //int totalProcs = ProcessManager::getProcessCount();
        //int activeProcs = ProcessManager::getRunningProcessCount(); // You implement this
        //int inactiveProcs = totalProcs - activeProcs;

        //size_t totalMem = config.maxOverallMem;
        //size_t usedMem = memMgr.getUsedMemory();
        //size_t freeMem = memMgr.getFreeMemory();

        //size_t totalFrames = memMgr.getTotalFrames();
        //size_t usedFrames = memMgr.getUsedFrames();

        //std::cout << "========= vmstat =========\n";
        //std::cout << "Processes:   total=" << totalProcs
        //    << "  active=" << activeProcs
        //    << "  inactive=" << inactiveProcs << "\n";

        //std::cout << "Memory:      total=" << totalMem
        //    << "B  used=" << usedMem
        //    << "B  free=" << freeMem << "B\n";

        //std::cout << "Frames:      total=" << totalFrames
        //    << "  used=" << usedFrames
        //    << "  free=" << (totalFrames - usedFrames) << "\n";

        //std::cout << "===========================\n";
    }

    else if (cmd == "report-util") {
        generateReport();
    }

    else if (cmd == "help") {
        showHelp();
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
