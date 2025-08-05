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

// Background Colors
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"
#define BG_BRIGHT_BLACK "\033[100m"
#define BG_BRIGHT_RED "\033[101m"
#define BG_BRIGHT_GREEN "\033[102m"
#define BG_BRIGHT_YELLOW "\033[103m"
#define BG_BRIGHT_BLUE "\033[104m"
#define BG_BRIGHT_MAGENTA "\033[105m"
#define BG_BRIGHT_CYAN "\033[106m"
#define BG_BRIGHT_WHITE "\033[107m"

#define CREAM "\033[38;5;230m"  // Light cream color

CLIManager::CLIManager() : schedulerThread(), generating(false) {}
extern std::atomic<int> totalCpuTicks;
extern std::atomic<int> activeCpuTicks;
extern std::atomic<int> idleCpuTicks;

void printBanner() {
    std::cout << BRIGHT_CYAN << BOLD;
    std::cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    std::cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    std::cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    std::cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    std::cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    std::cout << RESET << BRIGHT_YELLOW << BOLD;
    std::cout << "-------------------------------------------------------------\n";
    std::cout << RESET << BRIGHT_WHITE << BOLD << "Welcome to CSOPESY Emulator!\n" << RESET;
    std::cout << BRIGHT_GREEN << "\nDevelopers:\n" << RESET;
    std::cout << CREAM << "Hanna Angela D. De Los Santos\n";
    std::cout << "Joseph Dean T. Enriquez\n";
    std::cout << "Shanette Giane G. Presas\n";
    std::cout << "Jersey K. To\n" << RESET;
    std::cout << BRIGHT_YELLOW << BOLD;
    std::cout << "-------------------------------------------------------------\n" << RESET;
}

void CLIManager::run() {
    printBanner();
    std::string input;
    std::cout << BRIGHT_WHITE << "Welcome to the CLI Scheduler. Type " << BRIGHT_CYAN << "'help'" << BRIGHT_WHITE << " for commands.\n" << RESET;
    while (true) {
        std::cout << CREAM << BOLD << "root" << RESET << BRIGHT_WHITE << ":\\> " << RESET;
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
        std::cout << BRIGHT_RED << "Please run " << BRIGHT_YELLOW << "'initialize'" << BRIGHT_RED << " first.\n" << RESET;
        return;
    }

    if (cmd == "initialize") {
        if (Config::getInstance().loadFromFile("config.txt")) {
            std::cout << BRIGHT_GREEN << BOLD << "Configuration loaded successfully:\n" << RESET;

            auto& config = Config::getInstance();
            std::cout << BRIGHT_CYAN << "  numCPU: " << BRIGHT_WHITE << config.numCPU << "\n";
            std::cout << BRIGHT_CYAN << "  scheduler: " << BRIGHT_WHITE << "\"" << config.scheduler << "\"\n";
            std::cout << BRIGHT_CYAN << "  quantumCycles: " << BRIGHT_WHITE << config.quantumCycles << "\n";
            std::cout << BRIGHT_CYAN << "  batchProcessFreq: " << BRIGHT_WHITE << config.batchProcessFreq << "\n";
            std::cout << BRIGHT_CYAN << "  minInstructions: " << BRIGHT_WHITE << config.minInstructions << "\n";
            std::cout << BRIGHT_CYAN << "  maxInstructions: " << BRIGHT_WHITE << config.maxInstructions << "\n";
            std::cout << BRIGHT_CYAN << "  delayPerInstruction: " << BRIGHT_WHITE << config.delayPerInstruction << "\n";
            std::cout << BRIGHT_CYAN << "  maxOverallMem: " << BRIGHT_WHITE << config.maxOverallMem << "\n";
            std::cout << BRIGHT_CYAN << "  memPerFrame: " << BRIGHT_WHITE << config.memPerFrame << "\n";
            std::cout << BRIGHT_CYAN << "  minMemPerProc: " << BRIGHT_WHITE << config.minMemPerProc << "\n";
            std::cout << BRIGHT_CYAN << "  maxMemPerProc: " << BRIGHT_WHITE << config.maxMemPerProc << "\n" << RESET;

            MemoryManager::getInstance().initialize();
            std::ofstream backingStore("csopesy-backing-store.txt", std::ios::app);
            backingStore.close();

            if (config.scheduler == "FCFS" || config.scheduler == "fcfs") {
                schedulerType = SchedulerType::FCFS;
            }
            else if (config.scheduler == "RR" || config.scheduler == "rr") {
                schedulerType = SchedulerType::ROUND_ROBIN;
            }
            else {
                std::cerr << BRIGHT_RED << "Unknown scheduler type in config: " << config.scheduler << ", defaulting to FCFS.\n" << RESET;
                schedulerType = SchedulerType::FCFS;
            }

            startScheduler(Config::getInstance());
            std::cout << BRIGHT_GREEN << BOLD << "Scheduler initialized and ready for processes.\n" << RESET;

            initialized = true;
        }
        else {
            std::cout << BRIGHT_RED << "Failed to load config.txt.\n" << RESET;
        }
    }

    // Enhanced screen command with proper memory validation
    else if (cmd == "screen" && tokens.size() == 4 && tokens[1] == "-s") {
        std::string name = tokens[2];
        int requestedMem = 0;

        try {
            requestedMem = std::stoi(tokens[3]);

            // Validate memory size
            if (requestedMem < 64 || requestedMem > 65536) {
                std::cout << BRIGHT_RED << "Error: Memory size must be between 64 and 65536 bytes.\n";
                std::cout << "invalid memory allocation\n" << RESET;
                return;
            }

            if ((requestedMem & (requestedMem - 1)) != 0) {
                std::cout << BRIGHT_RED << "Error: Memory size must be a power of 2.\n";
                std::cout << BRIGHT_YELLOW << "Valid sizes: 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536\n";
                std::cout << BRIGHT_RED << "invalid memory allocation\n" << RESET;
                return;
            }

        }
        catch (...) {
            std::cout << BRIGHT_RED << "Error: Invalid memory size format.\n";
            std::cout << "invalid memory allocation\n" << RESET;
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
                std::cout << BRIGHT_YELLOW << "Warning: Limited physical memory available (" << availableMemory
                    << " KB free). Process will use virtual memory.\n" << RESET;
            }

            proc = ProcessManager::createNamedProcess(name, requestedMem);
            ProcessManager::addProcess(proc);
            addProcess(proc);

            std::cout << BRIGHT_GREEN << BOLD << "Process created successfully:\n" << RESET;
            std::cout << BRIGHT_CYAN << "  Name: " << BRIGHT_WHITE << name << "\n";
            std::cout << BRIGHT_CYAN << "  PID: " << BRIGHT_WHITE << proc->pid << "\n";
            std::cout << BRIGHT_CYAN << "  Memory allocated: " << BRIGHT_WHITE << requestedMem << " bytes\n";
            std::cout << BRIGHT_CYAN << "  Required frames: " << BRIGHT_WHITE << requiredFrames << "\n";
            std::cout << BRIGHT_CYAN << "  Status: " << BRIGHT_GREEN << "Added to scheduler queue\n" << RESET;

            ConsoleView::show(proc);
        }
        else {
            std::cout << BRIGHT_RED << "Error: Process '" << name << "' already exists.\n" << RESET;
        }
    }

    // New screen -c command
    else if (cmd == "screen" && tokens.size() >= 4 && tokens[1] == "-c") {
        std::string name = tokens[2];
        int requestedMem = 0;
        size_t instructionStartIndex = 4; // Default: memory size provided

        // Check if memory size is provided or if we should use default
        bool hasMemorySize = true;
        try {
            requestedMem = std::stoi(tokens[3]);

            // Validate memory size
            if (requestedMem < 64 || requestedMem > 65536) {
                std::cout << BRIGHT_RED << "invalid command\n" << RESET;
                return;
            }

            if ((requestedMem & (requestedMem - 1)) != 0) {
                std::cout << BRIGHT_RED << "invalid command\n" << RESET;
                return;
            }
        }
        catch (...) {
            // Failed to parse as number
            hasMemorySize = false;
            requestedMem = Config::getInstance().maxMemPerProc;
            instructionStartIndex = 3; // Instructions start at 4th token instead
        }

        // Get instructions string
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
            // Trim whitespace
            instruction.erase(0, instruction.find_first_not_of(" \t"));
            instruction.erase(instruction.find_last_not_of(" \t") + 1);
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
        }

        // Validate instruction count (1-50)
        if (instructions.size() < 1 || instructions.size() > 50) {
            std::cout << BRIGHT_RED << "invalid command\n" << RESET;
            return;
        }

        auto proc = ProcessManager::findByName(name);
        if (!proc) {
            proc = ProcessManager::createProcessWithInstructions(name, requestedMem, instructions);
            ProcessManager::addProcess(proc);
            addProcess(proc);

            std::cout << BRIGHT_GREEN << BOLD << "Process created successfully:\n" << RESET;
            std::cout << BRIGHT_CYAN << "  Name: " << BRIGHT_WHITE << name << "\n";
            std::cout << BRIGHT_CYAN << "  PID: " << BRIGHT_WHITE << proc->pid << "\n";
            std::cout << BRIGHT_CYAN << "  Memory allocated: " << BRIGHT_WHITE << requestedMem << " bytes";
            if (!hasMemorySize) {
                std::cout << BRIGHT_YELLOW << " (default)";
            }
            std::cout << "\n";
            std::cout << BRIGHT_CYAN << "  Instructions: " << BRIGHT_WHITE << instructions.size() << "\n";
            std::cout << BRIGHT_CYAN << "  Status: " << BRIGHT_GREEN << "Added to scheduler queue\n" << RESET;

            ConsoleView::show(proc);
        }
        else {
            std::cout << BRIGHT_RED << "Error: Process '" << name << "' already exists.\n" << RESET;
        }
    }

    else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
        std::string name = tokens[2];
        auto proc = ProcessManager::findByName(name);

        if (!proc) {
            std::cout << BRIGHT_RED << "Process not found.\n" << RESET;
            return;
        }

        if (proc->isFinished && proc->hasMemoryViolation) {
            std::cout << BRIGHT_RED << "Process shut down due to memory access violation error that occurred at "
                << proc->memoryViolationAddress << ". invalid.\n" << RESET;
            return;
        }

        if (proc->isFinished) {
            std::cout << BRIGHT_RED << "Process not found.\n" << RESET;
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
                std::cout << BRIGHT_RED << "Process '" << name << "' not found.\n" << RESET;
                return;
            }
        }
        else {
            std::cout << BRIGHT_YELLOW << "Usage: process-smi [process_name]\n";
            std::cout << "  process-smi           - Show system-wide memory and process overview\n";
            std::cout << "  process-smi [name]    - Show detailed info for specific process\n" << RESET;
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
                    std::cerr << BRIGHT_RED << "[Scheduler Error] " << e.what() << "\n" << RESET;
                }
                });
            std::cout << BRIGHT_GREEN << "Batch process generation started.\n" << RESET;
        }
        else {
            std::cout << BRIGHT_YELLOW << "Batch process generation is already running.\n" << RESET;
        }
    }

    else if (cmd == "scheduler-stop") {
        if (generating) {
            generating = false;
            if (schedulerThread.joinable()) schedulerThread.join();
            std::cout << BRIGHT_GREEN << "Stopped generating batch processes.\n";
            std::cout << BRIGHT_CYAN << "Note: Scheduler continues running for manual processes.\n" << RESET;
        }
        else {
            std::cout << BRIGHT_YELLOW << "Batch process generation was not running.\n" << RESET;
        }
    }

    else if (cmd == "vmstat") {
        printVMStat();
    }

    else if (cmd == "memory-status") {
        MemoryManager::getInstance().printMemoryStatus();
    }

    else if (cmd == "clear-memory") {
        std::cout << BRIGHT_YELLOW << "Are you sure you want to clear all memory and backing store? (y/N): " << RESET;
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

            std::cout << BRIGHT_GREEN << "Memory and backing store cleared. Scheduler restarted.\n" << RESET;
        }
        else {
            std::cout << BRIGHT_CYAN << "Operation cancelled.\n" << RESET;
        }
    }

    else if (cmd == "report-util") {
        generateReport();
    }

    else if (cmd == "help") {
        showHelp();
    }

    else {
        std::cout << BRIGHT_RED << "Unknown command. Type " << BRIGHT_CYAN << "'help'" << BRIGHT_RED << " for available commands.\n" << RESET;
    }
}

void CLIManager::stopScheduler() {
    generating = false;
    if (schedulerThread.joinable()) schedulerThread.join();
    ::stopScheduler();
}

void CLIManager::showHelp() const {
    std::cout << BRIGHT_CYAN << BOLD << "Available commands:\n" << RESET;
    std::cout << BRIGHT_YELLOW << "  initialize" << BRIGHT_WHITE << "                    - Load config.txt, prepare memory, and start scheduler\n";
    std::cout << BRIGHT_YELLOW << "  screen -s [name] [memory_size]" << BRIGHT_WHITE << " - Create a process with specified memory (auto-scheduled)\n";
    std::cout << BRIGHT_YELLOW << "  screen -c [name] [memory_size] \"[instructions]\"" << BRIGHT_WHITE << " - Create process with user-defined instructions (auto-scheduled)\n";
    std::cout << BRIGHT_YELLOW << "  process-smi [name]" << BRIGHT_WHITE << "            - Show detailed information for a specific process\n";
    std::cout << BRIGHT_YELLOW << "  screen -r [name]" << BRIGHT_WHITE << "              - Resume and inspect a process\n";
    std::cout << BRIGHT_YELLOW << "  screen -ls" << BRIGHT_WHITE << "                    - Show running and finished processes\n";
    std::cout << BRIGHT_YELLOW << "  scheduler-start" << BRIGHT_WHITE << "               - Begin automatic batch process generation\n";
    std::cout << BRIGHT_YELLOW << "  scheduler-stop" << BRIGHT_WHITE << "                - Stop automatic batch process generation\n";
    std::cout << BRIGHT_YELLOW << "  vmstat" << BRIGHT_WHITE << "                        - Show virtual memory statistics\n";
    std::cout << BRIGHT_YELLOW << "  memory-status" << BRIGHT_WHITE << "                 - Show detailed memory manager status\n";
    std::cout << BRIGHT_YELLOW << "  report-util" << BRIGHT_WHITE << "                   - Generate utilization report\n";
    std::cout << BRIGHT_YELLOW << "  clear-memory" << BRIGHT_WHITE << "                  - Clear all memory and backing store (debugging)\n";
    std::cout << BRIGHT_YELLOW << "  exit" << BRIGHT_WHITE << "                          - Exit the CLI\n\n";

    std::cout << BRIGHT_GREEN << BOLD << "Memory allocation examples:\n" << RESET;
    std::cout << BRIGHT_CYAN << "  screen -s myprocess 256" << BRIGHT_WHITE << "       - Allocate 256 bytes (auto-scheduled)\n";
    std::cout << BRIGHT_CYAN << "  screen -c process2 512 \"DECLARE varA 10; ADD varA varA varB\"" << BRIGHT_WHITE << " - Create with instructions (auto-scheduled)\n\n";

    std::cout << BRIGHT_MAGENTA << BOLD << "Note: " << BRIGHT_WHITE << "Scheduler starts automatically with 'initialize'. Manual processes are scheduled immediately.\n";
    std::cout << "      'scheduler-start' only controls automatic batch process generation.\n" << RESET;
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
        std::cout << BRIGHT_RED << "Error: numCPU is 0. Please check your config.txt file.\n" << RESET;
        return;
    }

    double utilization = (static_cast<double>(running) / numCPU) * 100.0;
    std::cout << std::fixed << std::setprecision(1);
    std::cout << BRIGHT_CYAN << BOLD << "CPU utilization: " << BRIGHT_WHITE << std::min(utilization, 100.0) << "%\n";
    std::cout << BRIGHT_CYAN << "Cores used: " << BRIGHT_WHITE << running << "\n";
    std::cout << BRIGHT_CYAN << "Cores available: " << BRIGHT_WHITE << std::max(0, numCPU - running) << "\n";
    std::cout << BRIGHT_CYAN << "Total processes: " << BRIGHT_WHITE << all.size() << "\n";
    std::cout << BRIGHT_CYAN << "Running: " << BRIGHT_GREEN << running << BRIGHT_CYAN << " | Waiting: " << BRIGHT_YELLOW << waiting << BRIGHT_CYAN << " | Finished: " << BRIGHT_MAGENTA << finished << "\n\n" << RESET;

    std::cout << BRIGHT_GREEN << BOLD << "Running processes:\n" << RESET;
    for (const auto& p : all) {
        if (p->isRunning && !p->isFinished) {
            std::cout << BRIGHT_WHITE << "  " << p->name << " (PID:" << p->pid << ") | Core " << p->coreAssigned
                << " | " << *p->completedInstructions << "/" << p->instructions.size()
                << " | Started: " << p->startTime << "\n" << RESET;
        }
    }

    std::cout << BRIGHT_YELLOW << BOLD << "\nWaiting processes:\n" << RESET;
    for (const auto& p : all) {
        if (!p->isRunning && !p->isFinished) {
            std::cout << BRIGHT_WHITE << "  " << p->name << " (PID:" << p->pid << ") | "
                << *p->completedInstructions << "/" << p->instructions.size() << " completed\n" << RESET;
        }
    }

    std::cout << BRIGHT_MAGENTA << BOLD << "\nFinished processes:\n" << RESET;
    for (const auto& p : all) {
        if (p->isFinished) {
            std::cout << BRIGHT_WHITE << "  " << p->name << " (PID:" << p->pid << ") | Finished: " << p->endTime
                << " | " << *p->completedInstructions << "/" << p->instructions.size() << " completed\n" << RESET;
        }
    }
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

    std::cout << BRIGHT_CYAN << BOLD << "VMSTAT:\n" << RESET;
    std::cout << BRIGHT_YELLOW << "  Total memory     : " << BRIGHT_WHITE << totalMem << " KB\n";
    std::cout << BRIGHT_YELLOW << "  Used memory      : " << BRIGHT_WHITE << usedMem << " KB\n";
    std::cout << BRIGHT_YELLOW << "  Free memory      : " << BRIGHT_WHITE << freeMem << " KB\n";
    std::cout << BRIGHT_YELLOW << "  Idle CPU ticks   : " << BRIGHT_WHITE << idleCpuTicks.load() << "\n";
    std::cout << BRIGHT_YELLOW << "  Active CPU ticks : " << BRIGHT_WHITE << activeCpuTicks.load() << "\n";
    std::cout << BRIGHT_YELLOW << "  Total CPU ticks  : " << BRIGHT_WHITE << totalCpuTicks.load() << "\n";
    std::cout << BRIGHT_YELLOW << "  Num paged in     : " << BRIGHT_WHITE << mem.getPageIns() << "\n";
    std::cout << BRIGHT_YELLOW << "  Num paged out    : " << BRIGHT_WHITE << mem.getPageOuts() << "\n" << RESET;
}
void CLIManager::printSystemProcessSMI() {
    auto& config = Config::getInstance();
    auto& mem = MemoryManager::getInstance();
    auto allProcesses = ProcessManager::getAllProcesses();

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
    std::cout << BRIGHT_CYAN << BOLD << std::string(80, '=') << "\n";
    std::cout << "                           CSOPESY PROCESS-SMI\n";
    std::cout << std::string(80, '=') << "\n" << RESET;
    std::cout << BRIGHT_WHITE << "Generated: " << timestamp << "\n\n" << RESET;

    // Driver and System Info
    std::cout << BRIGHT_GREEN << "Driver Version: CSOPESY v1.0    Scheduler: " << BRIGHT_YELLOW << config.scheduler;
    if (config.scheduler == "RR" || config.scheduler == "rr") {
        std::cout << " (Quantum: " << config.quantumCycles << ")";
    }
    std::cout << "\n\n" << RESET;

    // CPU Information
    std::cout << BRIGHT_CYAN << BOLD << "CPU Information:\n" << RESET;
    std::cout << BRIGHT_WHITE << "+----------------------+----------+----------+----------+\n";
    std::cout << "| CPU Cores            | Total    | Running  | Idle     |\n";
    std::cout << "+----------------------+----------+----------+----------+\n";
    std::cout << "| Count                | " << std::setw(8) << totalCores
        << " | " << std::setw(8) << runningProcesses
        << " | " << std::setw(8) << (totalCores - runningProcesses) << " |\n";
    std::cout << "| Utilization          | " << std::setw(6) << std::fixed << std::setprecision(1)
        << cpuUtilization << "% | " << std::setw(6) << (100.0 - cpuUtilization) << "% | N/A      |\n";
    std::cout << "+----------------------+----------+----------+----------+\n\n" << RESET;

    // Memory Information
    std::cout << BRIGHT_CYAN << BOLD << "Memory Information:\n" << RESET;
    std::cout << BRIGHT_WHITE << "+----------------------+----------+----------+----------+\n";
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
    std::cout << "+----------------------+----------+----------+----------+\n\n" << RESET;

    // Process Overview
    std::cout << BRIGHT_CYAN << BOLD << "Process Overview:\n" << RESET;
    std::cout << BRIGHT_WHITE << "+----------+----------+----------+----------+\n";
    std::cout << "| Total    | Running  | Waiting  | Finished |\n";
    std::cout << "+----------+----------+----------+----------+\n";
    std::cout << "| " << std::setw(8) << allProcesses.size()
        << " | " << std::setw(8) << runningProcesses
        << " | " << std::setw(8) << waitingProcesses
        << " | " << std::setw(8) << finishedProcesses << " |\n";
    std::cout << "+----------+----------+----------+----------+\n\n" << RESET;

    // Running Processes Detail
    if (runningProcesses > 0) {
        std::cout << BRIGHT_GREEN << BOLD << "Running Processes:\n" << RESET;
        std::cout << BRIGHT_WHITE << "+------+------------------+------+--------+----------+----------+----------+\n";
        std::cout << "| Core | Process Name     | PID  | Memory | Progress | CPU Time | Status   |\n";
        std::cout << "+------+------------------+------+--------+----------+----------+----------+\n";

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

            std::cout << "| " << std::setw(4) << proc->coreAssigned
                << " | " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(6) << processMemoryUsage << " | "
                << std::setw(6) << std::fixed << std::setprecision(1) << progress << "% | "
                << std::setw(8) << *proc->completedInstructions << " | "
                << std::setw(8) << BRIGHT_GREEN << "RUNNING" << BRIGHT_WHITE << " |\n";
        }
        std::cout << "+------+------------------+------+--------+----------+----------+----------+\n\n" << RESET;
    }

    if (waitingProcesses > 0) {
        std::cout << BRIGHT_YELLOW << BOLD << "Waiting Processes:\n" << RESET;
        std::cout << BRIGHT_WHITE << "+------------------+------+--------+----------+----------+\n";
        std::cout << "| Process Name     | PID  | Memory | Progress | Status   |\n";
        std::cout << "+------------------+------+--------+----------+----------+\n";

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

            std::string status = "WAITING";
            if (proc->hasMemoryViolation) {
                status = "ERROR";
            }

            std::cout << "| " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(6) << processMemoryUsage << " | "
                << std::setw(6) << std::fixed << std::setprecision(1) << progress << "% | "
                << std::setw(8) << BRIGHT_YELLOW << status << BRIGHT_WHITE << " |\n";
        }
        std::cout << "+------------------+------+--------+----------+----------+\n\n" << RESET;
    }

    auto finishedProcs = ProcessManager::getFinishedProcesses();
    if (!finishedProcs.empty()) {
        std::cout << BRIGHT_MAGENTA << BOLD << "Recently Finished Processes (last 5):\n" << RESET;
        std::cout << BRIGHT_WHITE << "+------------------+------+----------+----------+----------+\n";
        std::cout << "| Process Name     | PID  | Memory   | Completed| Status   |\n";
        std::cout << "+------------------+------+----------+----------+----------+\n";

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

            std::cout << "| " << std::setw(16) << std::left << displayName
                << " | " << std::setw(4) << std::right << proc->pid
                << " | " << std::setw(8) << proc->virtualMemoryLimit << " | "
                << std::setw(8) << *proc->completedInstructions << " | "
                << std::setw(8) << BRIGHT_MAGENTA << status << BRIGHT_WHITE << " |\n";
        }
        std::cout << "+------------------+------+----------+----------+----------+\n\n" << RESET;
    }

    std::cout << BRIGHT_CYAN << BOLD << "Performance Statistics:\n" << RESET;
    std::cout << BRIGHT_WHITE << "+------------------------+----------+\n";
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
    std::cout << "+------------------------+----------+\n\n" << RESET;

    // Footer
    std::cout << BRIGHT_CYAN << BOLD << std::string(80, '=') << "\n";
    std::cout << BRIGHT_WHITE << "Use 'process-smi [process_name]' for detailed process information\n";
    std::cout << "Use 'screen -ls' to see detailed process list with timestamps\n";
    std::cout << BRIGHT_CYAN << BOLD << std::string(80, '=') << "\n" << RESET;

}
