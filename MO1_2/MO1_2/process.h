#ifndef PROCESS_H
#define PROCESS_H

#include "MemoryManager.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <mutex>
#include <fstream>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

class Instruction;

enum class ProcessStatus {
    READY,
    RUNNING,
    DONE
};

struct Process {
    int pid = -1;  //  Initialized
    std::string name;
    int instructionPointer = 0;  //  Initialized
    std::vector<std::shared_ptr<Instruction>> instructions;
    std::vector<char> memory;
    std::unordered_map<size_t, size_t> pageTable; // virtualPageNumber -> frameNumber
    std::unordered_set<size_t> loadedPages;       // for quick lookup
    std::unordered_map<std::string, size_t> variableAddressMap;
    std::unordered_map<std::string, size_t> variableAddresses;

    bool isPageLoaded(size_t virtualPage) const {
        return loadedPages.count(virtualPage) > 0;
    }

    size_t getVariablePage(const std::string& varName) const {
        return variableAddressMap.at(varName) / MemoryManager::getInstance().getFrameSize();
    }

    int baseAddress = -1;
    size_t requiredMemory = 0;
    ProcessStatus status = ProcessStatus::READY;

    std::atomic<int> coreAssigned{ -1 };
    bool isRunning = false;
    bool isFinished = false;
    bool isDetached = false;

    std::string arrivalTime;
    std::string startTime;
    std::string endTime;
    std::atomic<int> wakeupTick{ 0 };

    int totalInstructions = 0;
    std::shared_ptr<std::atomic<int>> completedInstructions;

    std::vector<std::string> logs;
    mutable std::mutex logMutex;

    Process() : completedInstructions(std::make_shared<std::atomic<int>>(0)) {}

    void setBaseAddress(int address) { baseAddress = address; }
    int getBaseAddress() const { return baseAddress; }
    void setRequiredMemory(size_t memory) { requiredMemory = memory; }
    size_t getRequiredMemory() const { return requiredMemory; }

    void setStatus(ProcessStatus newStatus) {
        status = newStatus;
        if (newStatus == ProcessStatus::DONE) {
            isFinished = true;
        }
    }
    ProcessStatus getStatus() const { return status; }
    bool getIsFinished() const { return isFinished || status == ProcessStatus::DONE; }

    void setWakeupTick(int tick) { wakeupTick = tick; }
    int getWakeupTick() const { return wakeupTick.load(); }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &time_t);

        char timeBuffer[100];
        strftime(timeBuffer, sizeof(timeBuffer), "[%m/%d/%Y %I:%M:%S%p]", &tm);
        std::string logEntry = std::string(timeBuffer) + " " + message;
        logs.push_back(logEntry);
    }

    void writeLogToFile() const {
        std::lock_guard<std::mutex> lock(logMutex);
        std::string filename = name + "_log.txt";
        std::ofstream file(filename);

        if (!file.is_open()) return;

        file << "Process Log for " << name << " (PID: " << pid << ")\n";
        file << "===========================================\n\n";

        for (const auto& logEntry : logs) {
            file << logEntry << "\n";
        }

        file.close();
    }

    double getProgress() const {
        if (totalInstructions == 0) return 0.0;
        return (static_cast<double>(completedInstructions->load()) / totalInstructions) * 100.0;
    }

    int getRemainingInstructions() const {
        return totalInstructions - completedInstructions->load();
    }

    bool declareVariable(const std::string& name, uint16_t value) {
        if (variableAddressMap.size() >= 32) return false;
        size_t addr = 0x0000 + (variableAddressMap.size() * 2);
        variableAddressMap[name] = addr;

        memory[addr] = static_cast<char>(value >> 8);      // high byte
        memory[addr + 1] = static_cast<char>(value & 0xFF); // low byte
        return true;
    }

    void unloadPage(size_t page);
    void loadPage(size_t page, size_t frame);

    // Inside class Process in Process.h
    void writeMemory(size_t address, uint16_t value);
    uint16_t readMemory(size_t address) const;



private:
    static constexpr size_t symbolTableBase = 0x0000;
};

std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns, size_t memPerProc);

#endif // PROCESS_H
