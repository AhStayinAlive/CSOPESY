#include "CLIManager.h"
#include "config.h"
#include "ProcessManager.h"
#include "MemoryManager.h"
#include "ConsoleView.h"
#include "scheduler.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

CLIManager::CLIManager() : schedulerThread(), generating(false) {}
extern std::atomic<int> totalCpuTicks;
extern std::atomic<int> activeCpuTicks;
extern std::atomic<int> idleCpuTicks;

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
            MemoryManager::getInstance().initialize();
            std::ofstream backingStore("csopesy-backing-store.txt", std::ios::app);
            backingStore.close();

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

    // Enhanced screen command with proper memory validation
    else if (cmd == "screen" && tokens.size() == 4 && tokens[1] == "-s") {
        std::string name = tokens[2];
        int requestedMem = 0;

        try {
            requestedMem = std::stoi(tokens[3]);

            // Validate memory size: must be power of 2 between 64 and 65536
            if (requestedMem < 64 || requestedMem > 65536) {
                std::cout << "Error: Memory size must be between 64 and 65536 bytes.\n";
                std::cout << "invalid memory allocation\n";
                return;
            }

            // Check if it's a power of 2
            if ((requestedMem & (requestedMem - 1)) != 0) {
                std::cout << "Error: Memory size must be a power of 2.\n";
                std::cout << "Valid sizes: 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536\n";
                std::cout << "invalid memory allocation\n";
                return;
            }

        }
        catch (...) {
            std::cout << "Error: Invalid memory size format.\n";
            std::cout << "invalid memory allocation\n";
            return;
        }

        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            // Check if we have enough physical memory available
            auto& mem = MemoryManager::getInstance();
            auto& config = Config::getInstance();

            int availableMemory = (mem.getTotalFrames() - mem.getUsedFrames()) * config.memPerFrame;
            int requiredFrames = (requestedMem + config.memPerFrame - 1) / config.memPerFrame; // Ceiling division

            if (availableMemory < requestedMem && !mem.hasFreeFrame()) {
                std::cout << "Warning: Limited physical memory available (" << availableMemory
                    << " KB free). Process will use virtual memory.\n";
            }

            proc = ProcessManager::createNamedProcess(name, requestedMem);
            ProcessManager::addProcess(proc);
            addProcess(proc);

            std::cout << "Process created successfully:\n";
            std::cout << "  Name: " << name << "\n";
            std::cout << "  PID: " << proc->pid << "\n";
            std::cout << "  Memory allocated: " << requestedMem << " bytes\n";
            std::cout << "  Required frames: " << requiredFrames << "\n";

            ConsoleView::show(proc);
        }
        else {
            std::cout << "Error: Process '" << name << "' already exists.\n";
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

    // Enhanced process-smi command
    else if (cmd == "process-smi" && tokens.size() == 2) {
        std::string name = tokens[1];
        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            std::cout << "Process '" << name << "' not found.\n";
            return;
        }

        std::cout << "=== PROCESS SYSTEM MANAGEMENT INTERFACE ===\n";
        std::cout << "Process Information:\n";
        std::cout << "  Name                : " << proc->name << "\n";
        std::cout << "  PID                 : " << proc->pid << "\n";
        std::cout << "  Status              : ";

        if (proc->isFinished) {
            std::cout << "FINISHED\n";
        }
        else if (proc->isRunning) {
            std::cout << "RUNNING (Core " << proc->coreAssigned << ")\n";
        }
        else {
            std::cout << "WAITING\n";
        }

        std::cout << "  Instructions        : " << *proc->completedInstructions << " / " << proc->instructions.size();
        if (proc->instructions.size() > 0) {
            double progress = (static_cast<double>(*proc->completedInstructions) / proc->instructions.size()) * 100.0;
            std::cout << " (" << std::fixed << std::setprecision(1) << progress << "%)";
        }
        std::cout << "\n";

        if (!proc->startTime.empty()) {
            std::cout << "  Start Time          : " << proc->startTime << "\n";
        }
        if (!proc->endTime.empty()) {
            std::cout << "  End Time            : " << proc->endTime << "\n";
        }

        // Memory information
        int frameSize = Config::getInstance().memPerFrame;
        int totalVirtualPages = (proc->virtualMemoryLimit + frameSize - 1) / frameSize; // Ceiling division
        int pagesInMemory = 0;
        int pagesInBacking = 0;

        for (const auto& [vpn, entry] : proc->pageTable) {
            if (entry.valid) pagesInMemory++;
            else pagesInBacking++;
        }

        std::cout << "\nMemory Information:\n";
        std::cout << "  Virtual Memory Limit: " << proc->virtualMemoryLimit << " bytes\n";
        std::cout << "  Page Size           : " << frameSize << " bytes\n";
        std::cout << "  Total Virtual Pages : " << totalVirtualPages << "\n";
        std::cout << "  Pages in Memory     : " << pagesInMemory << "\n";
        std::cout << "  Pages in Backing    : " << pagesInBacking << "\n";
        std::cout << "  Memory Utilization  : " << (pagesInMemory * frameSize) << " bytes";
        if (proc->virtualMemoryLimit > 0) {
            double memUsage = (static_cast<double>(pagesInMemory * frameSize) / proc->virtualMemoryLimit) * 100.0;
            std::cout << " (" << std::fixed << std::setprecision(1) << memUsage << "%)";
        }
        std::cout << "\n";

        // Variable allocation table
        if (!proc->variableTable.empty()) {
            std::cout << "\nVariable Allocation:\n";
            std::cout << "  Variable Name       | Virtual Address | Page | Offset\n";
            std::cout << "  --------------------|-----------------|------|-------\n";

            for (const auto& [varName, address] : proc->variableTable) {
                int page = address / frameSize;
                int offset = address % frameSize;
                std::cout << "  " << std::setw(19) << std::left << varName.substr(0, 19) << " | "
                    << std::setw(15) << std::right << address << " | "
                    << std::setw(4) << page << " | "
                    << std::setw(6) << offset << "\n";
            }
        }

        // Recent logs (last 10)
        std::cout << "\nRecent Logs (last 10):\n";
        if (proc->logs.empty()) {
            std::cout << "  [No logs available]\n";
        }
        else {
            int startIdx = std::max(0, static_cast<int>(proc->logs.size()) - 10);
            for (int i = startIdx; i < static_cast<int>(proc->logs.size()); ++i) {
                std::cout << "  " << proc->logs[i] << "\n";
            }
        }
        std::cout << "==========================================\n";
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

    else if (cmd == "vmstat") {
        printVMStat();
    }

    // New command: memory-status
    else if (cmd == "memory-status") {
        MemoryManager::getInstance().printMemoryStatus();

        std::cout << "\nBacking Store Contents:\n";
        std::ifstream backingStore("csopesy-backing-store.txt");
        if (backingStore.is_open()) {
            std::string line;
            int lineCount = 0;
            std::cout << "Recent entries (last 15):\n";
            std::vector<std::string> lines;

            // Read all lines
            while (std::getline(backingStore, line)) {
                if (!line.empty() && line[0] != '#') {
                    lines.push_back(line);
                }
            }

            // Show last 15 lines
            int startIdx = std::max(0, static_cast<int>(lines.size()) - 15);
            for (int i = startIdx; i < static_cast<int>(lines.size()); ++i) {
                std::cout << "  " << lines[i] << "\n";
                lineCount++;
            }

            if (lineCount == 0) {
                std::cout << "  [No entries in backing store]\n";
            }
            else {
                std::cout << "  Total entries: " << lines.size() << "\n";
            }
            backingStore.close();
        }
        else {
            std::cout << "  [Backing store file not accessible]\n";
        }
    }

    // New command: clear-memory (for debugging)
    else if (cmd == "clear-memory") {
        std::cout << "Are you sure you want to clear all memory and backing store? (y/N): ";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm == "y" || confirm == "Y") {
            // Stop scheduler first
            if (generating) {
                generating = false;
                if (schedulerThread.joinable()) schedulerThread.join();
            }

            // Clear all processes
            ProcessManager::clearAllProcesses();

            // Reinitialize memory
            MemoryManager::getInstance().initialize();

            std::cout << "Memory and backing store cleared.\n";
        }
        else {
            std::cout << "Operation cancelled.\n";
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
        << "  initialize                    - Load config.txt and prepare scheduler\n"
        << "  screen -s [name] [memory_size] - Create a process with specified memory (64-65536 bytes, power of 2)\n"
        << "  process-smi [name]            - Show detailed information for a specific process\n"
        << "  screen -r [name]              - Resume and inspect a process\n"
        << "  screen -ls                    - Show running and finished processes\n"
        << "  scheduler-start               - Begin periodic batch process generation\n"
        << "  scheduler-stop                - Stop batch process generation\n"
        << "  vmstat                        - Show virtual memory statistics\n"
        << "  memory-status                 - Show detailed memory manager status\n"
        << "  report-util                   - Generate utilization report\n"
        << "  clear-memory                  - Clear all memory and backing store (debugging)\n"
        << "  exit                          - Exit the CLI\n\n"
        << "Memory allocation examples:\n"
        << "  screen -s myprocess 256       - Allocate 256 bytes\n"
        << "  screen -s bigprocess 4096     - Allocate 4KB\n"
        << "  screen -s maxprocess 65536    - Allocate 64KB (maximum)\n";
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

////void CLIManager::printVMStat() {
//    auto& config = Config::getInstance();
//    auto& mem = MemoryManager::getInstance();
//
//    int totalMem = config.maxOverallMem;
//    int frameSize = config.memPerFrame;
//    int totalFrames = totalMem / frameSize;
//    int usedFrames = mem.getUsedFrames();
//
//    int usedMem = usedFrames * frameSize;
//    int freeMem = totalMem - usedMem;
//    double memUtilization = totalFrames > 0 ? (static_cast<double>(usedFrames) / totalFrames * 100.0) : 0.0;
//
//    std::cout << "=== VIRTUAL MEMORY STATISTICS ===\n";
//    std::cout << "Physical Memory:\n";
//    std::cout << "  Total memory        : " << totalMem << " KB\n";
//    std::cout << "  Used memory         : " << usedMem << " KB (" << std::fixed << std::setprecision(1) << memUtilization << "%)\n";
//    std::cout << "  Free memory         : " << freeMem << " KB\n";
//    std::cout << "  Frame size          : " << frameSize << " KB\n";
//    std::cout << "  Total frames        : " << totalFrames << "\n";
//    std::cout << "  Used frames         : " << usedFrames << "\n";
//    std::cout << "  Available frames    : " << (totalFrames - usedFrames) << "\n\n";
//
//    std::cout << "Paging Statistics:\n";
//    std::cout << "  Pages loaded (page-ins)  : " << mem.getPageIns() << "\n";
//    std::cout << "  Pages evicted (page-outs): " << mem.getPageOuts() << "\n\n";
//
//    std::cout << "CPU Statistics:\n";
//    std::cout << "  Idle CPU ticks      : " << idleCpuTicks.load() << "\n";
//    std::cout << "  Active CPU ticks    : " << activeCpuTicks.load() << "\n";
//    std::cout << "  Total CPU ticks     : " << totalCpuTicks.load() << "\n";
//
//    if (totalCpuTicks.load() > 0) {
//        double cpuUtilization = (static_cast<double>(activeCpuTicks.load()) / totalCpuTicks.load()) * 100.0;
//        std::cout << "  CPU utilization     : " << std::fixed << std::setprecision(1) << cpuUtilization << "%\n";
//    }
//
//    // Show process memory usage
//    auto allProcesses = ProcessManager::getAllProcesses();
//    if (!allProcesses.empty()) {
//        std::cout << "\nPer-Process Memory Usage:\n";
//        std::cout << "  PID  | Name           | Virtual | Pages in Memory | Pages in Backing\n";
//        std::cout << "  -----|----------------|---------|------------------|------------------\n";
//
//        for (const auto& proc : allProcesses) {
//            int pagesInMemory = 0;
//            int pagesInBacking = 0;
//
//            for (const auto& [vpn, entry] : proc->pageTable) {
//                if (entry.valid) pagesInMemory++;
//                else pagesInBacking++;
//            }
//
//            std::cout << "  " << std::setw(4) << proc->pid << " | "
//                << std::setw(14) << std::left << proc->name.substr(0, 14) << " | "
//                << std::setw(7) << std::right << proc->virtualMemoryLimit << " | "
//                << std::setw(16) << pagesInMemory << " | "
//                << std::setw(17) << pagesInBacking << "\n";
//        }
//    }
//    std::cout << "===================================\n";
//}

void CLIManager::printVMStat() {
    auto& config = Config::getInstance();
    auto& mem = MemoryManager::getInstance();

    int totalMem = config.maxOverallMem;
    int frameSize = config.memPerFrame;
    int totalFrames = totalMem / frameSize;

    int usedFrames = 0;
    for (int i = 0; i < totalFrames; ++i) {
        if (mem.isFrameOccupied(i)) usedFrames++;
    }

    int usedMem = usedFrames * frameSize;
    int freeMem = totalMem - usedMem;

    std::cout << "VMSTAT:\n";
    std::cout << "  Total memory     : " << totalMem << " KB\n";
    std::cout << "  Used memory      : " << usedMem << " KB\n";
    std::cout << "  Free memory      : " << freeMem << " KB\n";
    std::cout << "  Idle CPU ticks   : " << idleCpuTicks.load() << "\n";
    std::cout << "  Active CPU ticks : " << activeCpuTicks.load() << "\n";
    std::cout << "  Total CPU ticks  : " << totalCpuTicks.load() << "\n";
    std::cout << "  Num paged in     : " << mem.getPageIns() << "\n";
    std::cout << "  Num paged out    : " << mem.getPageOuts() << "\n";
}