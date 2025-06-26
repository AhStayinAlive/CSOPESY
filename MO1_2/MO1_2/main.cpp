#include "config.h"
#include "process.h"
#include "scheduler.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <fstream>

extern std::vector<std::shared_ptr<Process>> allProcesses;

void printHeader() {
    std::cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    std::cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    std::cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    std::cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    std::cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    std::cout << "----------------------------------------------------\n";
    std::cout << "Welcome to CSOPESY Emulator!\n";
    std::cout << "\nDevelopers:\n";
    std::cout << "Hanna Angela D. De Los Santos\n";
    std::cout << "Joseph Dean T. Enriquez\n";
    std::cout << "Shanette Giane G. Presas\n";
    std::cout << "Jersey K. To\n";
}

int main() {
    Config config;
    bool initialized = false;
    int pidCounter = 1;
    std::atomic<int> cpuCycles(0);

    std::string input;
    printHeader();

    while (true) {
        cpuCycles++;
        std::cout << "\n> ";
        getline(std::cin, input);

        if (input == "initialize") {
            if (loadConfig("config.txt", config)) {
                initialized = true;
                std::cout << "Configuration loaded.\n";
            }
            else {
                std::cout << "Failed to load config.txt\n";
            }
        }
        else if (input == "exit") {
            stopScheduler();
            std::cout << "Exiting...\n";
            break;
        }
        else if (input == "scheduler-start") {
            if (!initialized) {
                std::cout << "Please run initialize first.\n";
                continue;
            }
            startScheduler(config);
            std::thread([&]() {
                while (running) {
                    if (!std::cin) break;
                    auto p = generateRandomProcess(pidCounter++, config.minInstructions, config.maxInstructions);
                    addProcess(p);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * config.batchProcessFreq));
                }
                }).detach();
        }
        else if (input == "scheduler-stop") {
            stopScheduler();
        }
        else if (input == "report-util") {
            std::ofstream report("csopesy-log.txt");

            if (!report) {
                std::cout << "Failed to write report.\n";
                continue;
            }

            int usedCores = std::count_if(allProcesses.begin(), allProcesses.end(),
                [](auto p) { return p->isRunning && !p->isFinished; });

            report << "CSOPESY Emulator Report\n\n";
            report << "CPU utilization: " << (config.numCPU == 0 ? 0 : (usedCores * 100 / config.numCPU)) << "%\n";
            report << "Cores used: " << usedCores << "\n";
            report << "Cores available: " << config.numCPU - usedCores << "\n";
            report << "-------------------------------------------\n";

            report << "\nRunning processes:\n";
            for (auto& p : allProcesses) {
                if (p->isRunning && !p->isFinished) {
                    report << p->name << " (" << p->startTime << ") Core: " << p->coreAssigned
                        << " " << p->instructionPointer << " / " << p->totalInstructions << "\n";
                }
            }

            report << "\n\nFinished processes:\n";
            for (auto& p : allProcesses) {
                if (p->isFinished) {
                    report << p->name << " (" << p->startTime << ") Finished "
                        << p->totalInstructions << " / " << p->totalInstructions << "\n";
                }
            }

            report.close();
            std::cout << "Report generated at csopesy-log.txt!\n";
        }
        else if (input.rfind("screen -s ", 0) == 0 || input.rfind("screen -r ", 0) == 0) {
            std::string procName = input.substr(10);
            std::shared_ptr<Process> procPtr = nullptr;
            for (auto& p : allProcesses) {
                if (p->name == procName) {
                    procPtr = p;
                    break;
                }
            }
            if (!procPtr) {
                std::cout << "Process " << procName << " not found.\n";
                continue;
            }
            while (true) {
                std::cout << "\n[" << procName << "]> ";
                std::string screenCmd;
                std::getline(std::cin, screenCmd);

                if (screenCmd == "process-smi") {
                    std::ifstream log(procPtr->name + ".txt");
                    if (log.is_open()) {
                        std::string line;
                        while (std::getline(log, line)) {
                            std::cout << line << "\n";
                        }
                        log.close();
                    }
                    else {
                        std::cout << "(No log file found)\n";
                    }
                    if (procPtr->isDetached)
                        std::cout << "[DETACHED MODE]\n";

                    std::cout << "\n";
                    if (procPtr->isFinished)
                        std::cout << "Finished!\n";
                    else {
                        std::cout << "Current instruction line: " << procPtr->instructionPointer << "\n";
                        std::cout << "Lines of code: " << procPtr->totalInstructions << "\n";
                    }
                }
                else if (screenCmd == "exit") {
                    break;
                }
                else {
                    std::cout << "Unknown command.\n";
                }
            }
        }
        else if (input.rfind("screen -d ", 0) == 0) {
            std::string procName = input.substr(10);
            std::shared_ptr<Process> procPtr = nullptr;
            for (auto& p : allProcesses) {
                if (p->name == procName) {
                    procPtr = p;
                    break;
                }
            }
            if (!procPtr) {
                std::cout << "Process " << procName << " not found.\n";
            }
            else {
                procPtr->isDetached = true;
                std::cout << "Detached from process " << procName << ".\n";
            }
        }
        else if (input == "screen -ls") {
            int usedCores = std::count_if(allProcesses.begin(), allProcesses.end(),
                [](auto p) { return p->isRunning && !p->isFinished; });

            std::cout << "CPU utilization: " << (usedCores * 100 / config.numCPU) << "%\n";
            std::cout << "Cores used: " << usedCores << "\n";
            std::cout << "Cores available: " << config.numCPU - usedCores << "\n";

            std::cout << "-------------------------------------------\n";
            std::cout << "Running processes:\n";
            for (auto p : allProcesses) {
                if (p->isRunning && !p->isFinished)
                    std::cout << p->name << " (" << p->startTime << ") Core: " << p->coreAssigned
                    << " " << p->instructionPointer << " / " << p->totalInstructions << "\n";
            }
            std::cout << "\nFinished processes:\n";
            for (auto p : allProcesses) {
                if (p->isFinished)
                    std::cout << p->name << " (" << p->startTime << ") Finished "
                    << p->totalInstructions << " / " << p->totalInstructions << "\n";
            }
            std::cout << "-------------------------------------------\n";
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}