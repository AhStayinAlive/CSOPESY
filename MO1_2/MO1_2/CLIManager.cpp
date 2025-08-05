#include "CLIManager.h"
#include "config.h"
#include "ProcessManager.h"
#include "MemoryManager.h"
#include "ConsoleView.h"
#include "scheduler.h"
#include <regex>
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

            // **NEW: Start scheduler immediately during initialization**
            startScheduler(Config::getInstance());
            std::cout << "Scheduler initialized and ready for processes.\n";

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

            // Check if it follows min and max mem per proc
            // const auto& config = Config::getInstance();
            // if (requestedMem < config.minMemPerProc || requestedMem > config.maxMemPerProc) {
            //     std::cout << "Error: Memory size must be between " << config.minMemPerProc
            //               << " and " << config.maxMemPerProc << " bytes.\n";
            //     std::cout << "invalid memory allocation\n";
            //     return;
            // }

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
            // **MODIFIED: Process will be automatically scheduled since scheduler is already running**
            addProcess(proc);

            std::cout << "Process created successfully:\n";
            std::cout << "  Name: " << name << "\n";
            std::cout << "  PID: " << proc->pid << "\n";
            std::cout << "  Memory allocated: " << requestedMem << " bytes\n";
            std::cout << "  Required frames: " << requiredFrames << "\n";
            std::cout << "  Status: Added to scheduler queue\n";  // **NEW: Indicate it's queued**

            ConsoleView::show(proc);
        }
        else {
            std::cout << "Error: Process '" << name << "' already exists.\n";
        }
    }

    // New screen -c command for user-defined instructions
    else if (cmd == "screen" && tokens.size() >= 4 && tokens[1] == "-c") {
        std::string name = tokens[2];
        int requestedMem = 0;
        size_t instructionStartIndex = 4; // Default: memory size provided

        // Check if memory size is provided or if we should use default
        bool hasMemorySize = true;
        try {
            // Try to parse the 4th token as memory size
            requestedMem = std::stoi(tokens[3]);

            // Validate memory size: must be power of 2 between 64 and 65536
            if (requestedMem < 64 || requestedMem > 65536) {
                std::cout << "invalid command\n";
                return;
            }

            // Check if it's a power of 2
            if ((requestedMem & (requestedMem - 1)) != 0) {
                std::cout << "invalid command\n";
                return;
            }
        }
        catch (...) {
            // Failed to parse as number, assume no memory size provided
            hasMemorySize = false;
            requestedMem = Config::getInstance().maxMemPerProc;
            instructionStartIndex = 3; // Instructions start at 4th token instead
        }

        // Get instructions string (everything after the appropriate token)
        std::string instructionsStr;
        for (size_t i = instructionStartIndex; i < tokens.size(); ++i) {
            if (i > instructionStartIndex) instructionsStr += " ";
            instructionsStr += tokens[i];
        }

        // Remove outer quotes if present
        if (instructionsStr.length() >= 2 && instructionsStr.front() == '"' && instructionsStr.back() == '"') {
            instructionsStr = instructionsStr.substr(1, instructionsStr.length() - 2);
        }

        // Split by semicolon
        std::vector<std::string> instructions;
        std::stringstream ss(instructionsStr);
        std::string instruction;

        while (std::getline(ss, instruction, ';')) {
            // Trim whitespace
            instruction.erase(0, instruction.find_first_not_of(" \t"));
            instruction.erase(instruction.find_last_not_of(" \t") + 1);
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
        }

        // Validate instruction count (1-50)
        if (instructions.size() < 1 || instructions.size() > 50) {
            std::cout << "invalid command\n";
            return;
        }

        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            proc = ProcessManager::createProcessWithInstructions(name, requestedMem, instructions);
            ProcessManager::addProcess(proc);
            // **MODIFIED: Process will be automatically scheduled since scheduler is already running**
            addProcess(proc);

            std::cout << "Process created successfully:\n";
            std::cout << "  Name: " << name << "\n";
            std::cout << "  PID: " << proc->pid << "\n";
            std::cout << "  Memory allocated: " << requestedMem << " bytes";
            if (!hasMemorySize) {
                std::cout << " (default)";
            }
            std::cout << "\n";
            std::cout << "  Instructions: " << instructions.size() << "\n";
            std::cout << "  Status: Added to scheduler queue\n";  // **NEW: Indicate it's queued**

            ConsoleView::show(proc);
        }
        else {
            std::cout << "Error: Process '" << name << "' already exists.\n";
        }
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);

        if (!proc) {
            std::cout << "Process not found.\n";
            return;
        }

        // **NEW: Check if process finished due to memory access violation**
        if (proc->isFinished && proc->hasMemoryViolation) {
            std::cout << "Process shut down due to memory access violation error that occurred at "
                << proc->memoryViolationAddress << ". invalid.\n";
            return;
        }

        // **MODIFIED: Check if process is finished (but not due to memory violation)**
        if (proc->isFinished) {
            std::cout << "Process not found.\n";  // Treat finished processes as "not found"
            return;
        }

        // Process exists and is not finished - show the console
        ConsoleView::show(proc);
        }


    else if (cmd == "screen" && tokens.size() == 2 && tokens[1] == "-ls") {
        showProcessList();
    }

    else if (cmd == "process-smi") {
        if (tokens.size() == 1) {
            // **NEW: System-wide process-smi (no arguments)**
            printSystemProcessSMI();
        }
        else if (tokens.size() == 2) {
            // **EXISTING: Per-process process-smi [name]**
            std::string name = tokens[1];
            auto proc = ProcessManager::findByName(name);
            if (!proc) {
                std::cout << "Process '" << name << "' not found.\n";
                return;
            }

            // ... existing per-process code remains the same ...
        }
        else {
            std::cout << "Usage: process-smi [process_name]\n";
            std::cout << "  process-smi           - Show system-wide memory and process overview\n";
            std::cout << "  process-smi [name]    - Show detailed info for specific process\n";
        }
}

    // Enhanced process-smi command (unchanged)
    else if (cmd == "process-smi" && tokens.size() == 2) {
        // ... (rest of process-smi code remains the same)
    }

    // **MODIFIED: scheduler-start and scheduler-test now only control batch process generation**
    else if (cmd == "scheduler-start" || cmd == "scheduler-test") {
        if (!generating) {
            generating = true;
            // **CHANGED: Only start batch process generation thread, scheduler is already running**
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
        else {
            std::cout << "Batch process generation is already running.\n";
        }
    }

    else if (cmd == "scheduler-stop") {
        if (generating) {
            generating = false;
            if (schedulerThread.joinable()) schedulerThread.join();
            std::cout << "Stopped generating batch processes.\n";
            std::cout << "Note: Scheduler continues running for manual processes.\n";  // **NEW: Clarification**
        }
        else {
            std::cout << "Batch process generation was not running.\n";
        }
    }

    // Rest of the commands remain unchanged...
    else if (cmd == "vmstat") {
        printVMStat();
    }

    else if (cmd == "memory-status") {
        MemoryManager::getInstance().printMemoryStatus();
        // ... (rest of memory-status code)
    }

    else if (cmd == "clear-memory") {
        std::cout << "Are you sure you want to clear all memory and backing store? (y/N): ";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm == "y" || confirm == "Y") {
            // **MODIFIED: Stop batch generation and restart scheduler**
            if (generating) {
                generating = false;
                if (schedulerThread.joinable()) schedulerThread.join();
            }

            // Stop scheduler temporarily
            ::stopScheduler();

            // Clear all processes
            ProcessManager::clearAllProcesses();

            // Reinitialize memory
            MemoryManager::getInstance().initialize();

            // Restart scheduler
            startScheduler(Config::getInstance());

            std::cout << "Memory and backing store cleared. Scheduler restarted.\n";
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
    // Stop batch process generation
    generating = false;
    if (schedulerThread.joinable()) schedulerThread.join();

    // Stop the main scheduler
    ::stopScheduler();
}

void CLIManager::showHelp() const {
    std::cout << "Available commands:\n"
        << "  initialize                    - Load config.txt, prepare memory, and start scheduler\n"
        << "  screen -s [name] [memory_size] - Create a process with specified memory (auto-scheduled)\n"
        << "  screen -c [name] [memory_size] \"[instructions]\" - Create process with user-defined instructions (auto-scheduled)\n"
        << "  process-smi [name]            - Show detailed information for a specific process\n"
        << "  screen -r [name]              - Resume and inspect a process\n"
        << "  screen -ls                    - Show running and finished processes\n"
        << "  scheduler-start               - Begin automatic batch process generation\n"
        << "  scheduler-stop                - Stop automatic batch process generation\n"
        << "  vmstat                        - Show virtual memory statistics\n"
        << "  memory-status                 - Show detailed memory manager status\n"
        << "  report-util                   - Generate utilization report\n"
        << "  clear-memory                  - Clear all memory and backing store (debugging)\n"
        << "  exit                          - Exit the CLI\n\n"
        << "Memory allocation examples:\n"
        << "  screen -s myprocess 256       - Allocate 256 bytes (auto-scheduled)\n"
        << "  screen -c process2 512 \"DECLARE varA 10; ADD varA varA varB\" - Create with instructions (auto-scheduled)\n\n"
        << "Note: Scheduler starts automatically with 'initialize'. Manual processes are scheduled immediately.\n"
        << "      'scheduler-start' only controls automatic batch process generation.\n";
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

void CLIManager::printSystemProcessSMI() {
    auto& config = Config::getInstance();
    auto& mem = MemoryManager::getInstance();
    auto allProcesses = ProcessManager::getAllProcesses();

    // Get current timestamp
    std::string timestamp = getCurrentTimestamp();

    // Calculate memory statistics
    int totalMemory = config.maxOverallMem;
    int frameSize = config.memPerFrame;
    int totalFrames = totalMemory / frameSize;
    int usedFrames = mem.getUsedFrames();
    int freeFrames = totalFrames - usedFrames;
    int usedMemory = usedFrames * frameSize;
    int freeMemory = freeFrames * frameSize;

    // Calculate CPU statistics
    int totalCores = config.numCPU;
    int runningProcesses = ProcessManager::getRunningProcessCount();
    int waitingProcesses = static_cast<int>(ProcessManager::getWaitingProcesses().size());
    int finishedProcesses = ProcessManager::getFinishedProcessCount();

    double memUtilization = totalFrames > 0 ? (static_cast<double>(usedFrames) / totalFrames * 100.0) : 0.0;
    double cpuUtilization = totalCores > 0 ? (static_cast<double>(runningProcesses) / totalCores * 100.0) : 0.0;

    // Header
    std::cout << std::string(80, '=') << "\n";
    std::cout << "                           CSOPESY PROCESS-SMI\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "Generated: " << timestamp << "\n\n";

    // Driver and System Info
    std::cout << "Driver Version: CSOPESY v1.0    Scheduler: " << config.scheduler;
    if (config.scheduler == "RR" || config.scheduler == "rr") {
        std::cout << " (Quantum: " << config.quantumCycles << ")";
    }
    std::cout << "\n\n";

    // CPU Information
    std::cout << "CPU Information:\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| CPU Cores            | Total    | Running  | Idle     |\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| Count                | " << std::setw(8) << totalCores
        << " | " << std::setw(8) << runningProcesses
        << " | " << std::setw(8) << (totalCores - runningProcesses) << " |\n";
    std::cout << "| Utilization          | " << std::setw(6) << std::fixed << std::setprecision(1)
        << cpuUtilization << "% | " << std::setw(6) << (100.0 - cpuUtilization) << "% | N/A      |\n";
    std::cout << "+----------------------+----------+----------+----------+\n\n";

    // Memory Information
    std::cout << "Memory Information:\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| Memory (KB)          | Total    | Used     | Free     |\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| Physical Memory      | " << std::setw(8) << totalMemory
        << " | " << std::setw(8) << usedMemory
        << " | " << std::setw(8) << freeMemory << " |\n";
    std::cout << "| Memory Frames        | " << std::setw(8) << totalFrames
        << " | " << std::setw(8) << usedFrames
        << " | " << std::setw(8) << freeFrames << " |\n";
    std::cout << "| Utilization          | " << std::setw(6) << std::fixed << std::setprecision(1)
        << memUtilization << "% | N/A      | N/A      |\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| Frame Size: " << frameSize << " KB  |  Page-ins: " << std::setw(6) << mem.getPageIns()
        << "  |  Page-outs: " << std::setw(6) << mem.getPageOuts() << " |\n";
    std::cout << "+----------------------+----------+----------+----------+\n\n";

    // Process Overview
    std::cout << "Process Overview:\n";
    std::cout << "+----------+----------+----------+----------+\n";
    std::cout << "| Total    | Running  | Waiting  | Finished |\n";
    std::cout << "+----------+----------+----------+----------+\n";
    std::cout << "| " << std::setw(8) << allProcesses.size()
        << " | " << std::setw(8) << runningProcesses
        << " | " << std::setw(8) << waitingProcesses
        << " | " << std::setw(8) << finishedProcesses << " |\n";
    std::cout << "+----------+----------+----------+----------+\n\n";

    // Running Processes Detail
    if (runningProcesses > 0) {
        std::cout << "Running Processes:\n";
        std::cout << "+------+------------------+------+--------+----------+----------+----------+\n";
        std::cout << "| Core | Process Name     | PID  | Memory | Progress | CPU Time | Status   |\n";
        std::cout << "+------+------------------+------+--------+----------+----------+----------+\n";

        auto runningProcs = ProcessManager::getRunningProcesses();
        for (const auto& proc : runningProcs) {
            // Calculate memory usage
            int processMemoryUsage = 0;
            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) {
                    processMemoryUsage += frameSize;
                }
            }

            // Calculate progress
            double progress = proc->instructions.size() > 0 ?
                (static_cast<double>(*proc->completedInstructions) / proc->instructions.size() * 100.0) : 0.0;

            // Truncate long process names
            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::cout << "| " << std::setw(4) << proc->coreAssigned
                << " | " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(6) << processMemoryUsage << " | "
                << std::setw(6) << std::fixed << std::setprecision(1) << progress << "% | "
                << std::setw(8) << *proc->completedInstructions << " | "
                << std::setw(8) << "RUNNING" << " |\n";
        }
        std::cout << "+------+------------------+------+--------+----------+----------+----------+\n\n";
    }

    // Waiting Processes (if any)
    if (waitingProcesses > 0) {
        std::cout << "Waiting Processes:\n";
        std::cout << "+------------------+------+--------+----------+----------+\n";
        std::cout << "| Process Name     | PID  | Memory | Progress | Status   |\n";
        std::cout << "+------------------+------+--------+----------+----------+\n";

        auto waitingProcs = ProcessManager::getWaitingProcesses();
        for (const auto& proc : waitingProcs) {
            // Calculate memory usage
            int processMemoryUsage = 0;
            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) {
                    processMemoryUsage += frameSize;
                }
            }

            // Calculate progress
            double progress = proc->instructions.size() > 0 ?
                (static_cast<double>(*proc->completedInstructions) / proc->instructions.size() * 100.0) : 0.0;

            // Truncate long process names
            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::string status = "WAITING";
            if (proc->hasMemoryViolation) {
                status = "ERROR";
            }

            std::cout << "| " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(6) << processMemoryUsage << " | "
                << std::setw(6) << std::fixed << std::setprecision(1) << progress << "% | "
                << std::setw(8) << status << " |\n";
        }
        std::cout << "+------------------+------+--------+----------+----------+\n\n";
    }

    // Recent Finished Processes (last 5)
    auto finishedProcs = ProcessManager::getFinishedProcesses();
    if (!finishedProcs.empty()) {
        std::cout << "Recently Finished Processes (last 5):\n";
        std::cout << "+------------------+------+----------+----------+----------+\n";
        std::cout << "| Process Name     | PID  | Memory   | Completed| Status   |\n";
        std::cout << "+------------------+------+----------+----------+----------+\n";

        // Show last 5 finished processes
        int startIdx = std::max(0, static_cast<int>(finishedProcs.size()) - 5);
        for (int i = startIdx; i < static_cast<int>(finishedProcs.size()); ++i) {
            auto proc = finishedProcs[i];

            // Truncate long process names
            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::string status = "FINISHED";
            if (proc->hasMemoryViolation) {
                status = "MEM_ERROR";
            }
            else if (*proc->completedInstructions < static_cast<int>(proc->instructions.size())) {
                status = "ERROR";
            }

            std::cout << "| " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(8) << proc->virtualMemoryLimit << " | "
                << std::setw(8) << *proc->completedInstructions << " | "
                << std::setw(8) << status << " |\n";
        }
        std::cout << "+------------------+------+----------+----------+----------+\n\n";
    }

    // Performance Statistics
    std::cout << "Performance Statistics:\n";
    std::cout << "+------------------------+----------+\n";
    std::cout << "| Metric                 | Value    |\n";
    std::cout << "+------------------------+----------+\n";
    std::cout << "| Total CPU Ticks        | " << std::setw(8) << totalCpuTicks.load() << " |\n";
    std::cout << "| Active CPU Ticks       | " << std::setw(8) << activeCpuTicks.load() << " |\n";
    std::cout << "| Idle CPU Ticks         | " << std::setw(8) << idleCpuTicks.load() << " |\n";

    if (totalCpuTicks.load() > 0) {
        double overallCpuUtil = (static_cast<double>(activeCpuTicks.load()) / totalCpuTicks.load()) * 100.0;
        std::cout << "| Overall CPU Utilization| " << std::setw(6) << std::fixed << std::setprecision(1)
            << overallCpuUtil << "% |\n";
    }

    std::cout << "| Instruction Delay      | " << std::setw(6) << config.delayPerInstruction << "ms |\n";
    std::cout << "+------------------------+----------+\n\n";

    // Footer
    std::cout << std::string(80, '=') << "\n";
    std::cout << "Use 'process-smi [process_name]' for detailed process information\n";
    std::cout << "Use 'screen -ls' to see detailed process list with timestamps\n";
    std::cout << std::string(80, '=') << "\n";
}
