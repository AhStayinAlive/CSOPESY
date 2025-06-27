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
#include "instruction_executor.h"

using namespace std;

std::vector<std::shared_ptr<Process>> allProcesses;
std::mutex processQueueMutex;
queue<shared_ptr<Process>> processQueue;
bool emulatorRunning = true;
Config config;
bool initialized = false;
int pidCounter = 1;
int pNameCounter = 1;
int cpuTickCounter = 0;
mutex coreAssignMutex;

bool requireInit(const std::string& command) {
    if (!initialized) {
        cout << "Command '" << command << "' requires initialization. Run `initialize` first.\n";
        return false;
    }
    return true;
}

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

void simulateExecution(std::shared_ptr<Process> proc) {
    try {
        proc->startTime = getCurrentTimestamp();
        proc->isRunning = true;
        proc->isFinished = false;

        while (proc->instructionPointer < proc->instructions.size()) {
            const auto& ins = proc->instructions[proc->instructionPointer];

            bool success = executeSingleInstruction(proc, ins, proc->coreAssigned);
            if (!success) {
                logToFile(proc->name,
                    "Execution stopped early at instruction " + std::to_string(proc->instructionPointer + 1) +
                    " (" + ins.opcode + ")", proc->coreAssigned);
                break;
            }

            proc->instructionPointer++;
            std::this_thread::sleep_for(std::chrono::milliseconds(config.delayPerInstruction * 100));
        }

        proc->endTime = getCurrentTimestamp();
        proc->isRunning = false;
        proc->isFinished = true;
    }
    catch (const std::exception& e) {
        logToFile(proc->name,
            "Unhandled exception: " + std::string(e.what()), proc->coreAssigned);
        proc->isRunning = false;
        proc->isFinished = true;
    }
}


void dispatcher() {
    while (emulatorRunning) {
        int core = getAvailableCore();
        if (core != -1 && !processQueue.empty()) {
            if (config.scheduler == "FCFS" || config.scheduler == "fcfs") {
                auto proc = processQueue.front();
                processQueue.pop();
                proc->coreAssigned = core;

                //cout << "[DISPATCH] " << proc->name << " dispatched to core " << core << " (FCFS)\n";
                if (!proc) {
                    cout << "[ERROR] scheduler: trying to push nullptr process!" << endl;
                    continue;
                }


                thread([proc]() mutable {
                    proc->isRunning = true;
                    proc->startTime = getCurrentTimestamp();

                    //cout << "[EXEC] " << proc->name << " started (FCFS)\n";

                    while (proc->instructionPointer < proc->instructions.size()) {
                        const auto& ins = proc->instructions[proc->instructionPointer];
                        if (!executeSingleInstruction(proc, ins, proc->coreAssigned)) break;
                        proc->instructionPointer++;
                        (*proc->completedInstructions)++;
                        this_thread::sleep_for(chrono::milliseconds(config.delayPerInstruction * 100));
                    }

                    proc->isRunning = false;
                    proc->isFinished = true;
                    proc->endTime = getCurrentTimestamp();

                    //cout << "[FINISHED] " << proc->name << " finished at " << proc->endTime << " (FCFS)\n";

                    }).detach();
            }

            else if (config.scheduler == "RR" || config.scheduler == "rr") {
                std::shared_ptr<Process> proc;
                {
                    std::lock_guard<std::mutex> lock(processQueueMutex);
                    if (processQueue.empty())
                        continue;
                    proc = processQueue.front();
                    processQueue.pop();
                }

                if (!proc) {
                    std::cout << "[DISPATCH] nullptr process in queue — skipping.\n";
                    continue;
                }

                proc->coreAssigned = core;

                //cout << "[DISPATCH] " << proc->name << " dispatched to core " << core << " (RR)\n";

                std::thread([proc, core, quantum = config.quantumCycles]() mutable {
                    proc->isRunning = true;
                    if (proc->startTime.empty()) // only set start time the first time
                        proc->startTime = getCurrentTimestamp();

                    //cout << "[EXEC] " << proc->name << " starting at instruction "
                        //<< proc->instructionPointer << " / " << proc->instructions.size() << " (RR)\n";

                    int executed = 0;
                    while (executed < quantum &&
                        proc->instructionPointer < proc->instructions.size()) {
                        const auto& ins = proc->instructions[proc->instructionPointer];
                        if (!executeSingleInstruction(proc, ins, core)) break;
                        proc->instructionPointer++;
                        (*proc->completedInstructions)++;
                        executed++;
                        this_thread::sleep_for(std::chrono::milliseconds(config.delayPerInstruction * 100));
                    }

                    if (proc->instructionPointer > proc->instructions.size())
                        proc->instructionPointer = proc->instructions.size();
                    if (*proc->completedInstructions > proc->instructions.size())
                        *proc->completedInstructions = proc->instructions.size();


                    proc->isRunning = false;

                    /*cout << "[EXEC DONE] " << proc->name << " executed " << executed
                        << " instructions. Instruction pointer now at "
                        << proc->instructionPointer << " / " << proc->instructions.size() << " (RR)\n";*/

                    if (proc->instructionPointer < proc->instructions.size()) {
                        std::lock_guard<std::mutex> lock(processQueueMutex);
                        processQueue.push(proc);
                    }
                    else {
                        proc->isFinished = true;
                        proc->endTime = getCurrentTimestamp();
                    }

                    }).detach();
            }
        }
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}



void screenConsole(std::shared_ptr<Process> proc) {
    cout << "+--------------------------------------+\n";
    cout << "|         PROCESS CONSOLE VIEW        |\n";
    cout << "+--------------------------------------+\n";
    cout << "| Process Name : " << proc->name << "\n";
    cout << "| ID           : " << proc->pid << "\n";

    ifstream logFile(proc->name + ".txt");
    if (logFile.is_open()) {
        string line;
        while (getline(logFile, line)) {
            cout << line << "\n";
        }
    }
    else {
        cout << "| No logs yet. Process has not started executing.\n";
    }

    cout << "| Current instruction line: " << *proc->completedInstructions << "\n";
    cout << "| Lines of code: " << proc->instructions.size() << "\n";

    if (proc->isFinished) {
        if (*proc->completedInstructions == proc->instructions.size()) {
            cout << "\nFinished!\n\n";
        }
        else {
            cout << "\nExecution stopped early due to an error.\n";
            if (proc->instructionPointer < proc->instructions.size()) {
                cout << "Failed at instruction: " << proc->instructionPointer + 1
                    << " (" << proc->instructions[proc->instructionPointer].opcode << ")\n\n";
            }
            else {
                cout << "Instruction pointer out of bounds.\n\n";
            }
        }
    }

    while (true) {
        cout << "root:\\> ";
        string input;
        getline(cin, input);

        if (input == "process-smi") {
            ifstream log(proc->name + ".txt");
            if (log.is_open()) {
                string line;
                while (getline(log, line)) {
                    cout << line << "\n";
                }
                log.close();
            }
            else {
                cout << "| No logs yet. Process has not started executing.\n";
            }

            cout << "Current instruction line: " << *proc->completedInstructions << "\n";
            cout << "Lines of code: " << proc->instructions.size() << "\n";

            if (proc->isFinished && *proc->completedInstructions == proc->instructions.size()) {
                cout << "\nFinished!\n\n";
            }
            else if (proc->isFinished && *proc->completedInstructions != proc->instructions.size()) {
                cout << "\nExecution stopped early due to an error.\n";
                if (proc->instructionPointer < proc->instructions.size()) {
                    cout << "Failed at instruction: " << proc->instructionPointer + 1
                        << " (" << proc->instructions[proc->instructionPointer].opcode << ")\n\n";
                }
                else {
                    cout << "Instruction pointer out of bounds.\n\n";
                }
            }

        }
        else if (input == "exit") {
            break;
        }
        else if (!input.empty()) {
            cout << "Unknown command.\n";
        }
    }
}

atomic<bool> generatingProcesses(false);
thread schedulerThread;

int main() {
    time_t t = time(0);
    srand(static_cast<unsigned int>(t));

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
                initialized = true;
                cout << "Configuration loaded.\n";
            }
            else {
                cout << "Failed to load config.txt.\n";
            }
        }
        else if (input == "screen -ls") {
            if (!requireInit("screen -ls")) continue;
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
                if (p->isFinished) {
                    cout << p->name << " (" << p->endTime << ") Finished "
                        << *p->completedInstructions << " / " << p->instructions.size() << "\n";
                }

        }
        else if (input.rfind("screen -s ", 0) == 0) {
            if (!requireInit("screen -s")) continue;
            string name = input.substr(10);
            shared_ptr<Process> proc = nullptr;

            for (auto& p : allProcesses) {
                string existingName = p->name;
                std::transform(existingName.begin(), existingName.end(), existingName.begin(), ::tolower);
                if (existingName == name) {
                    proc = p;
                    break;
                }
            }

            if (!proc) {
                system("CLS");
                proc = generateRandomProcess(name, pidCounter++, config.minInstructions, config.maxInstructions);
                allProcesses.push_back(proc);
                processQueue.push(proc);
                cout << "Process " << name << " created and queued.\n";
            }
            else {
                cout << "Process " << name << " already exists. Opening screen...\n";
            }

            screenConsole(proc);
        }
        else if (input.rfind("screen -r ", 0) == 0) {
            if (!requireInit("screen -r")) continue;
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
            if (!requireInit("scheduler-start")) continue;
            if (!generatingProcesses) {
                generatingProcesses = true;
                schedulerThread = thread([&] {
                    while (generatingProcesses) {
                        string candidateName;
                        shared_ptr<Process> proc;
                        while (true) {
                            candidateName = "p" + to_string(pNameCounter);
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
