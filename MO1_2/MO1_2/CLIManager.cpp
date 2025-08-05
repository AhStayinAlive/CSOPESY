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

// Enhanced visual elements (Windows compatible)
const std::string HEADER_LINE = std::string(80, '=');
const std::string SUB_HEADER_LINE = std::string(60, '-');
const std::string LIGHT_LINE = std::string(40, '.');

void printBanner() {
    std::cout << "\n" << HEADER_LINE << "\n";
    std::cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    std::cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    std::cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    std::cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    std::cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    std::cout << HEADER_LINE << "\n";
    std::cout << "             Welcome to CSOPESY Operating System Emulator!\n";
    std::cout << SUB_HEADER_LINE << "\n";
    std::cout << "Developers:\n";
    std::cout << "  * Hanna Angela D. De Los Santos\n";
    std::cout << "  * Joseph Dean T. Enriquez\n";
    std::cout << "  * Shanette Giane G. Presas\n";
    std::cout << "  * Jersey K. To\n";
    std::cout << HEADER_LINE << "\n\n";
}

void printSuccess(const std::string& message) {
    std::cout << "[+] " << message << "\n";
}

void printError(const std::string& message) {
    std::cout << "[-] " << message << "\n";
}

void printWarning(const std::string& message) {
    std::cout << "[!] " << message << "\n";
}

void printInfo(const std::string& message) {
    std::cout << "[i] " << message << "\n";
}

void printSectionHeader(const std::string& title) {
    std::cout << "\n" << SUB_HEADER_LINE << "\n";
    std::cout << "  " << title << "\n";
    std::cout << SUB_HEADER_LINE << "\n";
}

void CLIManager::run() {
    printBanner();
    std::string input;

    printInfo("System ready. Type 'help' to see available commands.");
    printWarning("Please run 'initialize' first to start the system.");

    while (true) {
        std::cout << "\nroot:\\> ";
        std::getline(std::cin, input);
        if (input == "exit") {
            std::cout << "\n" << LIGHT_LINE << "\n";
            printInfo("Shutting down CSOPESY...");
            stopScheduler();
            printSuccess("System shutdown complete. Goodbye!");
            std::cout << LIGHT_LINE << "\n";
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
        printError("System not initialized. Please run 'initialize' first.");
        return;
    }

    if (cmd == "initialize") {
        std::cout << "\n" << LIGHT_LINE << "\n";
        printInfo("Initializing CSOPESY system...");

        if (Config::getInstance().loadFromFile("config.txt")) {
            printSuccess("Configuration loaded successfully");

            MemoryManager::getInstance().initialize();
            printSuccess("Memory manager initialized");

            std::ofstream backingStore("csopesy-backing-store.txt", std::ios::app);
            backingStore.close();
            printSuccess("Backing store initialized");

            auto& config = Config::getInstance();
            if (config.scheduler == "FCFS" || config.scheduler == "fcfs") {
                schedulerType = SchedulerType::FCFS;
                printInfo("Scheduler type: First Come First Serve (FCFS)");
            }
            else if (config.scheduler == "RR" || config.scheduler == "rr") {
                schedulerType = SchedulerType::ROUND_ROBIN;
                printInfo("Scheduler type: Round Robin (RR) with quantum " + std::to_string(config.quantumCycles));
            }
            else {
                printWarning("Unknown scheduler type '" + config.scheduler + "', defaulting to FCFS");
                schedulerType = SchedulerType::FCFS;
            }

            startScheduler(Config::getInstance());
            printSuccess("Scheduler started and ready for processes");

            initialized = true;
            std::cout << LIGHT_LINE << "\n";
            printSuccess("System initialization complete!");
            printInfo("You can now create processes and start scheduling.");
        }
        else {
            printError("Failed to load config.txt file");
            printWarning("Please ensure config.txt exists in the current directory");
        }
    }

    // Enhanced screen command with better feedback
    else if (cmd == "screen" && tokens.size() == 4 && tokens[1] == "-s") {
        std::string name = tokens[2];
        int requestedMem = 0;

        try {
            requestedMem = std::stoi(tokens[3]);

            if (requestedMem < 64 || requestedMem > 65536) {
                printError("Memory size must be between 64 and 65536 bytes");
                printInfo("Example: screen -s myprocess 512");
                return;
            }

            if ((requestedMem & (requestedMem - 1)) != 0) {
                printError("Memory size must be a power of 2");
                printInfo("Valid sizes: 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536");
                return;
            }

        }
        catch (...) {
            printError("Invalid memory size format");
            printInfo("Usage: screen -s <process_name> <memory_size>");
            return;
        }

        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            auto& mem = MemoryManager::getInstance();
            auto& config = Config::getInstance();

            int availableMemory = (mem.getTotalFrames() - mem.getUsedFrames()) * config.memPerFrame;
            int requiredFrames = (requestedMem + config.memPerFrame - 1) / config.memPerFrame;

            if (availableMemory < requestedMem && !mem.hasFreeFrame()) {
                printWarning("Limited physical memory available (" + std::to_string(availableMemory) + " KB free)");
                printInfo("Process will use virtual memory if needed");
            }

            proc = ProcessManager::createNamedProcess(name, requestedMem);
            ProcessManager::addProcess(proc);
            addProcess(proc);

            printSectionHeader("Process Created Successfully");
            std::cout << "  Name: " << name << "\n";
            std::cout << "  PID: " << proc->pid << "\n";
            std::cout << "  Memory allocated: " << requestedMem << " bytes\n";
            std::cout << "  Required frames: " << requiredFrames << "\n";
            std::cout << "  Status: Added to scheduler queue\n";
            std::cout << SUB_HEADER_LINE << "\n";

            ConsoleView::show(proc);
        }
        else {
            printError("Process '" + name + "' already exists");
            printInfo("Use 'screen -r " + name + "' to resume the existing process");
        }
    }

    // Enhanced screen -c command
    else if (cmd == "screen" && tokens.size() >= 4 && tokens[1] == "-c") {
        std::string name = tokens[2];
        int requestedMem = 0;
        size_t instructionStartIndex = 4;
        bool hasMemorySize = true;

        try {
            requestedMem = std::stoi(tokens[3]);
            if (requestedMem < 64 || requestedMem > 65536 || (requestedMem & (requestedMem - 1)) != 0) {
                printError("Invalid memory size specification");
                return;
            }
        }
        catch (...) {
            hasMemorySize = false;
            requestedMem = Config::getInstance().maxMemPerProc;
            instructionStartIndex = 3;
        }

        std::string instructionsStr;
        for (size_t i = instructionStartIndex; i < tokens.size(); ++i) {
            if (i > instructionStartIndex) instructionsStr += " ";
            instructionsStr += tokens[i];
        }

        if (instructionsStr.length() >= 2 && instructionsStr.front() == '"' && instructionsStr.back() == '"') {
            instructionsStr = instructionsStr.substr(1, instructionsStr.length() - 2);
        }

        std::vector<std::string> instructions;
        std::stringstream ss(instructionsStr);
        std::string instruction;

        while (std::getline(ss, instruction, ';')) {
            instruction.erase(0, instruction.find_first_not_of(" \t"));
            instruction.erase(instruction.find_last_not_of(" \t") + 1);
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
        }

        if (instructions.size() < 1 || instructions.size() > 50) {
            printError("Invalid instruction count (must be 1-50 instructions)");
            printInfo("Separate instructions with semicolons");
            return;
        }

        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            proc = ProcessManager::createProcessWithInstructions(name, requestedMem, instructions);
            ProcessManager::addProcess(proc);
            addProcess(proc);

            printSectionHeader("Custom Process Created Successfully");
            std::cout << "  Name: " << name << "\n";
            std::cout << "  PID: " << proc->pid << "\n";
            std::cout << "  Memory allocated: " << requestedMem << " bytes";
            if (!hasMemorySize) std::cout << " (default)";
            std::cout << "\n";
            std::cout << "  Instructions: " << instructions.size() << "\n";
            std::cout << "  Status: Added to scheduler queue\n";
            std::cout << SUB_HEADER_LINE << "\n";

            ConsoleView::show(proc);
        }
        else {
            printError("Process '" + name + "' already exists");
        }
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);

        if (!proc) {
            printError("Process '" + name + "' not found");
            printInfo("Use 'screen -ls' to see available processes");
            return;
        }

        if (proc->isFinished && proc->hasMemoryViolation) {
            printError("Process terminated due to memory access violation at " + proc->memoryViolationAddress);
            printWarning("Cannot resume crashed process");
            return;
        }

        if (proc->isFinished) {
            printError("Process '" + name + "' has already finished");
            printInfo("Use 'screen -ls' to see active processes");
            return;
        }

        ConsoleView::show(proc);
    }

    else if (cmd == "screen" && tokens.size() == 2 && tokens[1] == "-ls") {
        showProcessList();
    }

    else if (cmd == "process-smi") {
        if (tokens.size() == 1) {
            printSystemProcessSMI();
        }
        else if (tokens.size() == 2) {
            std::string name = tokens[1];
            auto proc = ProcessManager::findByName(name);
            if (!proc) {
                printError("Process '" + name + "' not found");
                return;
            }
            // Process-specific SMI would be implemented here
        }
        else {
            printError("Invalid usage");
            printInfo("Usage: process-smi [process_name]");
        }
    }

    else if (cmd == "scheduler-start" || cmd == "scheduler-test") {
        if (!generating) {
            generating = true;
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
            printSuccess("Batch process generation started");
            printInfo("Processes will be created every " + std::to_string(Config::getInstance().batchProcessFreq) + " seconds");
        }
        else {
            printWarning("Batch process generation is already running");
        }
    }

    else if (cmd == "scheduler-stop") {
        if (generating) {
            generating = false;
            if (schedulerThread.joinable()) schedulerThread.join();
            printSuccess("Batch process generation stopped");
            printInfo("Manual process creation is still available");
        }
        else {
            printWarning("Batch process generation was not running");
        }
    }

    else if (cmd == "vmstat") {
        printVMStat();
    }

    else if (cmd == "memory-status") {
        printSectionHeader("Memory Manager Status");
        MemoryManager::getInstance().printMemoryStatus();
    }

    else if (cmd == "clear-memory") {
        printWarning("This will clear ALL memory and backing store data!");
        std::cout << "Are you sure you want to continue? (y/N): ";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm == "y" || confirm == "Y") {
            if (generating) {
                generating = false;
                if (schedulerThread.joinable()) schedulerThread.join();
            }

            ::stopScheduler();
            ProcessManager::clearAllProcesses();
            MemoryManager::getInstance().initialize();
            startScheduler(Config::getInstance());

            printSuccess("Memory and backing store cleared");
            printSuccess("Scheduler restarted");
        }
        else {
            printInfo("Operation cancelled");
        }
    }

    else if (cmd == "report-util") {
        printInfo("Generating system utilization report...");
        generateReport();
        printSuccess("Report saved to 'csopesy-log.txt'");
    }

    else if (cmd == "help") {
        showHelp();
    }

    else {
        printError("Unknown command: '" + cmd + "'");
        printInfo("Type 'help' for available commands");
    }
}

void CLIManager::stopScheduler() {
    generating = false;
    if (schedulerThread.joinable()) schedulerThread.join();
    ::stopScheduler();
}

void CLIManager::showHelp() const {
    printSectionHeader("CSOPESY Command Reference");

    std::cout << "\nSYSTEM COMMANDS:\n";
    std::cout << "  initialize                           Initialize system and start scheduler\n";
    std::cout << "  help                                 Show this help menu\n";
    std::cout << "  exit                                 Exit the system\n";

    std::cout << "\nPROCESS MANAGEMENT:\n";
    std::cout << "  screen -s <name> <memory_size>       Create process with auto-generated instructions\n";
    std::cout << "  screen -c <name> [size] \"<instrs>\"    Create process with custom instructions\n";
    std::cout << "  screen -r <name>                     Resume/inspect a process\n";
    std::cout << "  screen -ls                           List all processes\n";

    std::cout << "\nMONITORING:\n";
    std::cout << "  process-smi                          System-wide process and memory overview\n";
    std::cout << "  process-smi <name>                   Detailed info for specific process\n";
    std::cout << "  vmstat                               Virtual memory statistics\n";
    std::cout << "  memory-status                        Detailed memory manager status\n";
    std::cout << "  report-util                          Generate utilization report\n";

    std::cout << "\nSCHEDULER CONTROL:\n";
    std::cout << "  scheduler-start                      Begin automatic batch process generation\n";
    std::cout << "  scheduler-stop                       Stop automatic batch process generation\n";

    std::cout << "\nDEBUGGING:\n";
    std::cout << "  clear-memory                         Clear all memory and restart (debugging)\n";

    std::cout << "\nEXAMPLES:\n";
    std::cout << "  screen -s myprocess 512              Create process with 512 bytes memory\n";
    std::cout << "  screen -c calc 256 \"DECLARE x 5; ADD x x result; PRINT result\"\n";
    std::cout << "  process-smi                          Show system overview\n";

    std::cout << "\nNOTES:\n";
    std::cout << "  * Scheduler starts automatically with 'initialize'\n";
    std::cout << "  * Manual processes are scheduled immediately\n";
    std::cout << "  * Memory sizes must be powers of 2 (64-65536 bytes)\n";
    std::cout << "  * Custom instructions: DECLARE, ADD, SUBTRACT, PRINT, READ, WRITE\n";

    std::cout << "\n" << SUB_HEADER_LINE << "\n";
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
        printError("numCPU is 0. Please check your config.txt file");
        return;
    }

    double utilization = (static_cast<double>(running) / numCPU) * 100.0;

    printSectionHeader("Process List & System Status");

    // System overview
    std::cout << "SYSTEM OVERVIEW:\n";
    std::cout << "  CPU Utilization    : " << std::fixed << std::setprecision(1) << std::min(utilization, 100.0) << "%\n";
    std::cout << "  Cores in use       : " << running << "/" << numCPU << "\n";
    std::cout << "  Total processes    : " << all.size() << "\n";
    std::cout << "  Running            : " << running << "\n";
    std::cout << "  Waiting in queue   : " << waiting << "\n";
    std::cout << "  Finished           : " << finished << "\n";

    // Running processes
    if (running > 0) {
        std::cout << "\n[RUNNING] PROCESSES:\n";
        std::cout << "  " << std::setw(20) << std::left << "Name"
            << std::setw(8) << "PID"
            << std::setw(8) << "Core"
            << std::setw(12) << "Progress"
            << "Started\n";
        std::cout << "  " << std::string(60, '-') << "\n";

        for (const auto& p : all) {
            if (p->isRunning && !p->isFinished) {
                std::string progress = std::to_string(*p->completedInstructions) + "/" + std::to_string(p->instructions.size());
                std::cout << "  " << std::setw(20) << std::left << p->name.substr(0, 19)
                    << std::setw(8) << p->pid
                    << std::setw(8) << p->coreAssigned
                    << std::setw(12) << progress
                    << p->startTime << "\n";
            }
        }
    }

    // Waiting processes
    if (waiting > 0) {
        std::cout << "\n[WAITING] PROCESSES:\n";
        std::cout << "  " << std::setw(20) << std::left << "Name"
            << std::setw(8) << "PID"
            << "Progress\n";
        std::cout << "  " << std::string(40, '-') << "\n";

        for (const auto& p : all) {
            if (!p->isRunning && !p->isFinished) {
                std::string progress = std::to_string(*p->completedInstructions) + "/" + std::to_string(p->instructions.size());
                std::cout << "  " << std::setw(20) << std::left << p->name.substr(0, 19)
                    << std::setw(8) << p->pid
                    << progress;
                if (p->hasMemoryViolation) {
                    std::cout << " [MEMORY ERROR]";
                }
                std::cout << "\n";
            }
        }
    }

    // Finished processes (show last 10)
    if (finished > 0) {
        std::cout << "\n[FINISHED] PROCESSES";
        if (finished > 10) {
            std::cout << " (showing last 10 of " << finished << ")";
        }
        std::cout << ":\n";
        std::cout << "  " << std::setw(20) << std::left << "Name"
            << std::setw(8) << "PID"
            << std::setw(12) << "Progress"
            << "Finished\n";
        std::cout << "  " << std::string(60, '-') << "\n";

        std::vector<std::shared_ptr<Process>> finishedProcs;
        for (const auto& p : all) {
            if (p->isFinished) finishedProcs.push_back(p);
        }

        int startIdx = std::max(0, static_cast<int>(finishedProcs.size()) - 10);
        for (int i = startIdx; i < static_cast<int>(finishedProcs.size()); ++i) {
            auto p = finishedProcs[i];
            std::string progress = std::to_string(*p->completedInstructions) + "/" + std::to_string(p->instructions.size());
            std::cout << "  " << std::setw(20) << std::left << p->name.substr(0, 19)
                << std::setw(8) << p->pid
                << std::setw(12) << progress
                << p->endTime;
            if (p->hasMemoryViolation) {
                std::cout << " [MEMORY ERROR]";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n" << SUB_HEADER_LINE << "\n";
}

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
    double memUtilization = totalFrames > 0 ? (static_cast<double>(usedFrames) / totalFrames * 100.0) : 0.0;

    printSectionHeader("Virtual Memory Statistics");

    std::cout << "MEMORY STATUS:\n";
    std::cout << "  Total memory       : " << std::setw(8) << totalMem << " KB\n";
    std::cout << "  Used memory        : " << std::setw(8) << usedMem << " KB ("
        << std::fixed << std::setprecision(1) << memUtilization << "%)\n";
    std::cout << "  Free memory        : " << std::setw(8) << freeMem << " KB\n";
    std::cout << "  Frame size         : " << std::setw(8) << frameSize << " KB\n";
    std::cout << "  Total frames       : " << std::setw(8) << totalFrames << "\n";
    std::cout << "  Used frames        : " << std::setw(8) << usedFrames << "\n";
    std::cout << "  Available frames   : " << std::setw(8) << (totalFrames - usedFrames) << "\n";

    std::cout << "\nCPU STATISTICS:\n";
    std::cout << "  Idle CPU ticks     : " << std::setw(8) << idleCpuTicks.load() << "\n";
    std::cout << "  Active CPU ticks   : " << std::setw(8) << activeCpuTicks.load() << "\n";
    std::cout << "  Total CPU ticks    : " << std::setw(8) << totalCpuTicks.load() << "\n";

    if (totalCpuTicks.load() > 0) {
        double cpuUtilization = (static_cast<double>(activeCpuTicks.load()) / totalCpuTicks.load()) * 100.0;
        std::cout << "  CPU utilization    : " << std::setw(6) << std::fixed << std::setprecision(1)
            << cpuUtilization << "%\n";
    }

    std::cout << "\nPAGING STATISTICS:\n";
    std::cout << "  Pages loaded (in)  : " << std::setw(8) << mem.getPageIns() << "\n";
    std::cout << "  Pages evicted (out): " << std::setw(8) << mem.getPageOuts() << "\n";

    // Show per-process memory usage if processes exist
    auto allProcesses = ProcessManager::getAllProcesses();
    if (!allProcesses.empty()) {
        std::cout << "\nPER-PROCESS MEMORY USAGE:\n";
        std::cout << "  " << std::setw(6) << "PID"
            << std::setw(18) << "Name"
            << std::setw(10) << "Virtual"
            << std::setw(12) << "In Memory"
            << std::setw(12) << "In Backing" << "\n";
        std::cout << "  " << std::string(58, '-') << "\n";

        for (const auto& proc : allProcesses) {
            int pagesInMemory = 0;
            int pagesInBacking = 0;

            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) pagesInMemory++;
                else pagesInBacking++;
            }

            std::cout << "  " << std::setw(6) << proc->pid
                << std::setw(18) << proc->name.substr(0, 17)
                << std::setw(8) << proc->virtualMemoryLimit << " KB"
                << std::setw(10) << pagesInMemory << " pg"
                << std::setw(10) << pagesInBacking << " pg" << "\n";
        }
    }

    std::cout << "\n" << SUB_HEADER_LINE << "\n";
}

void CLIManager::printSystemProcessSMI() {
    auto& config = Config::getInstance();
    auto& mem = MemoryManager::getInstance();
    auto allProcesses = ProcessManager::getAllProcesses();

    std::string timestamp = getCurrentTimestamp();

    // Calculate statistics
    int totalMemory = config.maxOverallMem;
    int frameSize = config.memPerFrame;
    int totalFrames = totalMemory / frameSize;
    int usedFrames = mem.getUsedFrames();
    int freeFrames = totalFrames - usedFrames;
    int usedMemory = usedFrames * frameSize;
    int freeMemory = freeFrames * frameSize;

    int totalCores = config.numCPU;
    int runningProcesses = ProcessManager::getRunningProcessCount();
    int waitingProcesses = static_cast<int>(ProcessManager::getWaitingProcesses().size());
    int finishedProcesses = ProcessManager::getFinishedProcessCount();

    double memUtilization = totalFrames > 0 ? (static_cast<double>(usedFrames) / totalFrames * 100.0) : 0.0;
    double cpuUtilization = totalCores > 0 ? (static_cast<double>(runningProcesses) / totalCores * 100.0) : 0.0;

    std::cout << "                         CSOPESY PROCESS-SMI\n";
    std::cout << HEADER_LINE << "\n";
    std::cout << "Generated: " << timestamp << "\n\n";

    // Driver and System Info
    std::cout << "Driver Version: CSOPESY v1.0    Scheduler: " << config.scheduler;
    if (config.scheduler == "RR" || config.scheduler == "rr") {
        std::cout << " (Quantum: " << config.quantumCycles << ")";
    }
    std::cout << "\n\n";

    // CPU Information Section
    printSectionHeader("CPU Information");
    std::cout << std::setw(25) << std::left << "Metric"
        << std::setw(12) << "Total"
        << std::setw(12) << "Running"
        << std::setw(12) << "Idle" << "\n";
    std::cout << std::string(61, '-') << "\n";
    std::cout << std::setw(25) << "CPU Cores"
        << std::setw(12) << totalCores
        << std::setw(12) << runningProcesses
        << std::setw(12) << (totalCores - runningProcesses) << "\n";
    std::cout << std::setw(25) << "Utilization (%)"
        << std::setw(10) << std::fixed << std::setprecision(1) << cpuUtilization << "%"
        << std::setw(10) << (100.0 - cpuUtilization) << "%"
        << std::setw(12) << "N/A" << "\n";

    // Memory Information Section
    printSectionHeader("Memory Information");
    std::cout << std::setw(25) << std::left << "Metric"
        << std::setw(12) << "Total"
        << std::setw(12) << "Used"
        << std::setw(12) << "Free" << "\n";
    std::cout << std::string(61, '-') << "\n";
    std::cout << std::setw(25) << "Physical Memory (KB)"
        << std::setw(12) << totalMemory
        << std::setw(12) << usedMemory
        << std::setw(12) << freeMemory << "\n";
    std::cout << std::setw(25) << "Memory Frames"
        << std::setw(12) << totalFrames
        << std::setw(12) << usedFrames
        << std::setw(12) << freeFrames << "\n";
    std::cout << std::setw(25) << "Memory Utilization (%)"
        << std::setw(10) << std::fixed << std::setprecision(1) << memUtilization << "%"
        << std::setw(12) << "N/A"
        << std::setw(12) << "N/A" << "\n";

    std::cout << "\nFrame Size: " << frameSize << " KB  |  Page-ins: " << mem.getPageIns()
        << "  |  Page-outs: " << mem.getPageOuts() << "\n";

    // Process Overview Section
    printSectionHeader("Process Overview");
    std::cout << std::setw(15) << "Total"
        << std::setw(15) << "Running"
        << std::setw(15) << "Waiting"
        << std::setw(15) << "Finished" << "\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::setw(15) << allProcesses.size()
        << std::setw(15) << runningProcesses
        << std::setw(15) << waitingProcesses
        << std::setw(15) << finishedProcesses << "\n";

    // Running Processes Detail
    if (runningProcesses > 0) {
        printSectionHeader("Running Processes");
        std::cout << std::setw(6) << "Core"
            << std::setw(18) << "Process Name"
            << std::setw(8) << "PID"
            << std::setw(10) << "Memory"
            << std::setw(12) << "Progress"
            << std::setw(12) << "CPU Time"
            << "Status\n";
        std::cout << std::string(76, '-') << "\n";

        auto runningProcs = ProcessManager::getRunningProcesses();
        for (const auto& proc : runningProcs) {
            int processMemoryUsage = 0;
            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) {
                    processMemoryUsage += frameSize;
                }
            }

            double progress = proc->instructions.size() > 0 ?
                (static_cast<double>(*proc->completedInstructions) / proc->instructions.size() * 100.0) : 0.0;

            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::cout << std::setw(6) << proc->coreAssigned
                << std::setw(18) << displayName
                << std::setw(8) << proc->pid
                << std::setw(8) << processMemoryUsage << " KB"
                << std::setw(10) << std::fixed << std::setprecision(1) << progress << "%"
                << std::setw(12) << *proc->completedInstructions
                << "RUNNING\n";
        }
    }

    // Waiting Processes
    if (waitingProcesses > 0) {
        printSectionHeader("Waiting Processes");
        std::cout << std::setw(18) << "Process Name"
            << std::setw(8) << "PID"
            << std::setw(10) << "Memory"
            << std::setw(12) << "Progress"
            << "Status\n";
        std::cout << std::string(58, '-') << "\n";

        auto waitingProcs = ProcessManager::getWaitingProcesses();
        for (const auto& proc : waitingProcs) {
            int processMemoryUsage = 0;
            for (const auto& [vpn, entry] : proc->pageTable) {
                if (entry.valid) {
                    processMemoryUsage += frameSize;
                }
            }

            double progress = proc->instructions.size() > 0 ?
                (static_cast<double>(*proc->completedInstructions) / proc->instructions.size() * 100.0) : 0.0;

            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::string status = proc->hasMemoryViolation ? "MEM_ERROR" : "WAITING";

            std::cout << std::setw(18) << displayName
                << std::setw(8) << proc->pid
                << std::setw(8) << processMemoryUsage << " KB"
                << std::setw(10) << std::fixed << std::setprecision(1) << progress << "%"
                << status << "\n";
        }
    }

    // Recent Finished Processes
    auto finishedProcs = ProcessManager::getFinishedProcesses();
    if (!finishedProcs.empty()) {
        printSectionHeader("Recently Finished Processes (last 5)");
        std::cout << std::setw(18) << "Process Name"
            << std::setw(8) << "PID"
            << std::setw(12) << "Memory"
            << std::setw(12) << "Completed"
            << "Status\n";
        std::cout << std::string(60, '-') << "\n";

        int startIdx = std::max(0, static_cast<int>(finishedProcs.size()) - 5);
        for (int i = startIdx; i < static_cast<int>(finishedProcs.size()); ++i) {
            auto proc = finishedProcs[i];

            std::string displayName = proc->name.length() > 16 ?
                proc->name.substr(0, 13) + "..." : proc->name;

            std::string status = "FINISHED";
            if (proc->hasMemoryViolation) {
                status = "MEM_ERROR";
            }
            else if (*proc->completedInstructions < static_cast<int>(proc->instructions.size())) {
                status = "ERROR";
            }

            std::cout << std::setw(18) << displayName
                << std::setw(8) << proc->pid
                << std::setw(10) << proc->virtualMemoryLimit << " KB"
                << std::setw(12) << *proc->completedInstructions
                << status << "\n";
        }
    }

    // Performance Statistics
    printSectionHeader("Performance Statistics");
    std::cout << std::setw(30) << std::left << "Metric" << "Value\n";
    std::cout << std::string(40, '-') << "\n";
    std::cout << std::setw(30) << "Total CPU Ticks" << totalCpuTicks.load() << "\n";
    std::cout << std::setw(30) << "Active CPU Ticks" << activeCpuTicks.load() << "\n";
    std::cout << std::setw(30) << "Idle CPU Ticks" << idleCpuTicks.load() << "\n";

    if (totalCpuTicks.load() > 0) {
        double overallCpuUtil = (static_cast<double>(activeCpuTicks.load()) / totalCpuTicks.load()) * 100.0;
        std::cout << std::setw(30) << "Overall CPU Utilization"
            << std::fixed << std::setprecision(1) << overallCpuUtil << "%\n";
    }

    std::cout << std::setw(30) << "Instruction Delay" << config.delayPerInstruction << "ms\n";

    // Footer
    std::cout << "\n" << HEADER_LINE << "\n";
    printInfo("Use 'process-smi [process_name]' for detailed process information");
    printInfo("Use 'screen -ls' to see detailed process list with timestamps");
    std::cout << HEADER_LINE << "\n";
}