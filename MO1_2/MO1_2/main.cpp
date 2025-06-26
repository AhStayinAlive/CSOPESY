#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <map>
#include <mutex>
#include <queue>
#include "config.h"
#include "process.h"
#include "utils.h"
#include "scheduler.h"

using namespace std;

std::vector<std::shared_ptr<Process>> allProcesses;
queue<shared_ptr<Process>> processQueue;
bool emulatorRunning = true;
Config config;
bool initialized = false;
int pidCounter = 1;
int pNameCounter = 1;
int cpuTickCounter = 0;
mutex coreAssignMutex;

void printBanner() {
    cout << "  ____ _____   ____    ____  _____  _____  __    __\n";
    cout << " / ___/ ____| / __ \\  |  _ \\| ____|/ ____| \\ \\  / / \n";
    cout << "| |   \\___ \\ | /  \\ | | __)||  _|  \\___ \\   \\ \\/ /\n";
    cout << "| |___ ___) || \\__/ | | |   |  |__  ___) |   |  |\n";
    cout << " \\____|____/  \\____/  | |   |_____||____/    |__|\n";
    cout << "-------------------------------------------------------------\n";
    cout << "Welcome to CSOPESY Emulator!\n";
    cout << "Developers: \nDel Gallego, Neil Patrick\n\n";
    cout << "Last updated: 01-18-2024\n";
    cout << "-------------------------------------------------------------\n";
}

int getAvailableCore() {
    lock_guard<mutex> lock(coreAssignMutex);
    vector<int> used(config.numCPU, 0);
    for (auto& p : allProcesses) {
        if (p->isRunning && p->coreAssigned >= 0 && p->coreAssigned < config.numCPU)
            used[p->coreAssigned]++;
    }
    for (int i = 0; i < config.numCPU; ++i)
        if (used[i] == 0) return i;
    return -1;
}

void simulateExecution(shared_ptr<Process> proc) {
    try {
        proc->startTime = getCurrentTimestamp();
        proc->isRunning = true;
        auto& mem = proc->memory;

        for (int i = 0; i < proc->instructions.size(); ++i) {
            Instruction ins = proc->instructions[i];
            try {
                if (ins.opcode == "PRINT") {
                    logToFile(proc->name, ins.arg1, proc->coreAssigned);
                }
                else if (ins.opcode == "DECLARE") {
                    mem[ins.arg1] = static_cast<uint16_t>(std::stoi(ins.arg2));
                }
                else if (ins.opcode == "ADD") {
                    uint16_t left = mem.count(ins.arg2) ? mem[ins.arg2] : static_cast<uint16_t>(std::stoi(ins.arg2));
                    uint16_t right = mem.count(ins.arg3) ? mem[ins.arg3] : static_cast<uint16_t>(std::stoi(ins.arg3));
                    mem[ins.arg1] = left + right;
                }
                else if (ins.opcode == "SUBTRACT") {
                    uint16_t left = mem.count(ins.arg2) ? mem[ins.arg2] : static_cast<uint16_t>(std::stoi(ins.arg2));
                    uint16_t right = mem.count(ins.arg3) ? mem[ins.arg3] : static_cast<uint16_t>(std::stoi(ins.arg3));
                    mem[ins.arg1] = left - right;
                }
                else if (ins.opcode == "SLEEP") {
                    this_thread::sleep_for(chrono::milliseconds(500 * std::stoi(ins.arg1)));
                }
            }
            catch (const std::exception& e) {
                logToFile(proc->name, "Instruction error: " + std::string(e.what()), proc->coreAssigned);
                break; // Optional: skip entire process if instruction is invalid
            }

            this_thread::sleep_for(chrono::milliseconds(config.delayPerInstruction * 100));
            (*proc->completedInstructions)++;
        }

        proc->endTime = getCurrentTimestamp();
        proc->isRunning = false;
        proc->isFinished = true;
    }
    catch (const std::exception& e) {
        logToFile(proc->name, "Process execution error: " + std::string(e.what()), proc->coreAssigned);
        proc->isRunning = false;
        proc->isFinished = true;
    }
}


void dispatcher() {
    while (emulatorRunning) {
        if (!processQueue.empty()) {
            int core = getAvailableCore();
            if (core != -1) {
                auto proc = processQueue.front();
                processQueue.pop();
                proc->coreAssigned = core;
                thread(simulateExecution, proc).detach();
            }
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void screenConsole(shared_ptr<Process> proc) {
    cout << "+--------------------------------------+\n";
    cout << "|         PROCESS CONSOLE VIEW        |\n";
    cout << "+--------------------------------------+\n";
    cout << "| Process Name : " << proc->name << "\n";
    cout << "| ID           : " << proc->pid << "\n";
    cout << "| Logs:\n";

    ifstream logFile(proc->name + ".txt");
    string line;
    while (getline(logFile, line)) {
        if (line.find("Hello world") != string::npos)
            cout << line << "\n";
    }

    cout << "| Current instruction line: " << *proc->completedInstructions << "\n";
    cout << "| Lines of code: " << proc->instructions.size() << "\n";
    if (proc->isFinished) cout << "\nFinished!\n\n";

    while (true) {
        cout << "root:\\> ";
        string input;
        getline(cin, input);

        if (input == "process-smi") {
            cout << "\nProcess name: " << proc->name << "\nID: " << proc->pid << "\nLogs:\n";
            ifstream logFile(proc->name + ".txt");
            string line;
            while (getline(logFile, line)) {
                if (line.find("Hello world") != string::npos)
                    cout << line << "\n";
            }
            cout << "Current instruction line: " << *proc->completedInstructions << "\n";
            cout << "Lines of code: " << proc->instructions.size() << "\n";
            if (proc->isFinished) cout << "\nFinished!\n\n";
        }
        else if (input == "exit") break;
        else cout << "Unknown command.\n";
    }
}

atomic<bool> generatingProcesses(false);
thread schedulerThread;

int main() {
    srand(time(0));
    printBanner();

    thread tickThread([] {
        while (emulatorRunning) {
            this_thread::sleep_for(chrono::seconds(1));
            cpuTickCounter++;
        }
        });

    thread dispatcherThread(dispatcher);

    string input;
    while (emulatorRunning) {
        cout << "root:\\> ";
        getline(cin, input);

        if (input == "exit") {
            emulatorRunning = false;
            break;

        }
        else if (input == "initialize") {
            if (loadConfig("config.txt", config)) {
                cout << "Configuration loaded.\n";
                initialized = true;
            }
            else {
                cout << "Failed to load config.txt.\n";
            }

        }
        else if (input == "screen -ls") {
            int used = 0;
            for (auto& p : allProcesses)
                if (p->isRunning && !p->isFinished) used++;

            cout << "CPU utilization: " << (used * 100 / config.numCPU) << "%\n";
            cout << "Cores used: " << used << "\n";
            cout << "Cores available: " << max(0, config.numCPU - used) << "\n";

            cout << "\nRunning processes:\n";
            for (auto& p : allProcesses) {
                if (p->isRunning && !p->isFinished && p->coreAssigned != -1) {
                    cout << p->name << " (" << p->startTime << ") Core: " << p->coreAssigned
                        << " " << *p->completedInstructions << " / " << p->instructions.size() << "\n";
                }
            }


            cout << "\nFinished processes:\n";
            for (auto& p : allProcesses)
                if (p->isFinished)
                    cout << p->name << " (" << p->endTime << ") Finished "
                    << p->instructions.size() << " / " << p->instructions.size() << "\n";

        }
        else if (input.rfind("screen -s ", 0) == 0) {
            string name = input.substr(10);
            bool exists = false;
            for (auto& p : allProcesses) {
                if (p->name == name) {
                    cout << "Process already exists.\n";
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                system("CLS");
                auto proc = generateRandomProcess(name, pidCounter++, config.minInstructions, config.maxInstructions);
                allProcesses.push_back(proc);
                processQueue.push(proc);
                cout << "Process " << name << " queued.\n";
                screenConsole(proc);
            }
        }
        else if (input.rfind("screen -r ", 0) == 0) {
            string name = input.substr(10);
            bool found = false;
            for (auto& p : allProcesses) {
                if (p->name == name) {
                    screenConsole(p);
                    found = true;
                    break;
                }
            }
            if (!found) cout << "Process " << name << " not found.\n";

        }
        else if (input == "scheduler-start") {
            if (!generatingProcesses) {
                generatingProcesses = true;
                schedulerThread = thread([&] {
                    while (generatingProcesses) {
                        string candidateName;
                        shared_ptr<Process> proc;
                        while (true) {
                            candidateName = "p" + to_string(pNameCounter);

                            // Check if name exists
                            bool exists = false;
                            for (auto& p : allProcesses) {
                                if (p->name == candidateName) {
                                    exists = true;
                                    break;
                                }
                            }

                            if (!exists) {
                                proc = generateRandomProcess(candidateName, pidCounter++, config.minInstructions, config.maxInstructions);
                                ++pNameCounter;
                                break;
                            }
                            else {
                                ++pNameCounter;
                            }
                        }

                        allProcesses.push_back(proc);
                        processQueue.push(proc);
                        this_thread::sleep_for(chrono::seconds(config.batchProcessFreq));
                    }

                    });
                cout << "Scheduler started.\n";
            }
            else cout << "Scheduler already running.\n";

        }
        else if (input == "scheduler-stop") {
            if (generatingProcesses) {
                generatingProcesses = false;
                if (schedulerThread.joinable()) schedulerThread.join();
                cout << "Scheduler stopped.\n";
            }
            else cout << "Scheduler is not running.\n";

        }
        else {
            cout << "Unrecognized command.\n";
        }
    }

    if (dispatcherThread.joinable()) dispatcherThread.join();
    if (schedulerThread.joinable()) schedulerThread.join();
    tickThread.join();
    cout << "Exiting emulator...\n";
    return 0;
}