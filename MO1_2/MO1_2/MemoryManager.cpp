#include "MemoryManager.h"
#include "process.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>

// Static member definitions
std::unique_ptr<MemoryManager> MemoryManager::instance = nullptr;
std::once_flag MemoryManager::initFlag;

MemoryManager::MemoryManager() : memory(MEMORY_SIZE, '.') {
    std::cout << "MemoryManager initialized with " << MEMORY_SIZE << " bytes\n";
}

MemoryManager& MemoryManager::getInstance() {
    std::call_once(initFlag, []() {
        instance = std::unique_ptr<MemoryManager>(new MemoryManager());
        });
    return *instance;
}

bool MemoryManager::canAllocateAt(size_t startIndex, size_t size) const {
    if (startIndex + size > MEMORY_SIZE) return false;

    for (size_t i = startIndex; i < startIndex + size; ++i) {
        if (memory[i] != '.') return false;
    }
    return true;
}

void MemoryManager::allocateAt(size_t startIndex, size_t size) {
    for (size_t i = startIndex; i < startIndex + size; ++i) {
        memory[i] = '+';
    }
}

bool MemoryManager::allocate(std::shared_ptr<Process> process) {
    if (!process) return false;

    if (process->baseAddress != -1) return true;

    std::unique_lock<std::shared_mutex> lock(memoryMutex);

    for (size_t i = 0; i <= MEMORY_SIZE - process->requiredMemory; ++i) {
        if (canAllocateAt(i, process->requiredMemory)) {
            allocateAt(i, process->requiredMemory);
            process->setBaseAddress(static_cast<int>(i));

            std::string logMsg = "Memory allocated at address " + std::to_string(i) +
                " (size: " + std::to_string(process->requiredMemory) + ")";
            process->log(logMsg);

            std::cout << "Process " << process->name << " (PID: " << process->pid
                << ") allocated at address " << i << " (size: " << process->requiredMemory << ")\n";
            return true;
        }
    }

    process->log("Memory allocation failed - insufficient memory");
    std::cout << "Memory allocation failed for process " << process->name
        << " (PID: " << process->pid << ") - insufficient memory\n";
    return false;
}

void MemoryManager::deallocate(std::shared_ptr<Process> process) {
    if (!process || process->baseAddress == -1) return;

    std::unique_lock<std::shared_mutex> lock(memoryMutex);

    for (size_t i = process->baseAddress; i < process->baseAddress + process->requiredMemory; ++i) {
        if (i < MEMORY_SIZE) memory[i] = '.';
    }

    process->log("Memory deallocated from address " + std::to_string(process->baseAddress));
    std::cout << "Process " << process->name << " (PID: " << process->pid
        << ") deallocated from address " << process->baseAddress << "\n";

    process->setBaseAddress(-1);
}

void MemoryManager::visualizeMemory(int cycle) {
    std::string filename = "memory_stamp_" + std::to_string(cycle) + ".txt";
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create memory stamp file: " << filename << "\n";
        return;
    }

    std::shared_lock<std::shared_mutex> lock(memoryMutex);

    file << "Timestamp: (" << getCurrentTimestamp() << ")\n";
    file << "Number of processes in memory: " << getProcessesInMemory() << "\n";
    file << "Total external fragmentation in KB: " << (getExternalFragmentation() / 1024.0) << "\n\n";

    file << "Memory Layout (. = free, + = allocated):\n";
    const size_t lineWidth = 64;
    for (size_t i = 0; i < MEMORY_SIZE; i += lineWidth) {
        file << std::setfill('0') << std::setw(5) << i << ": ";
        size_t endIndex = std::min(i + lineWidth, MEMORY_SIZE);
        for (size_t j = i; j < endIndex; ++j) {
            file << memory[j];
        }
        file << "\n";
    }

    file << "\n";
    file << "Memory Statistics:\n";
    file << "Total Memory: " << MEMORY_SIZE << " bytes\n";
    file << "Used Memory: " << getUsedMemory() << " bytes\n";
    file << "Free Memory: " << getFreeMemory() << " bytes\n";
    file << "External Fragmentation: " << getExternalFragmentation() << " bytes\n";

    file.close();
}

void MemoryManager::incrementCycle() {
    int cycle = currentCycle.fetch_add(1);
    visualizeMemory(cycle);
}

std::string MemoryManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
    localtime_s(&tm, &time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p");
    return ss.str();
}

size_t MemoryManager::getUsedMemory() const {
    std::shared_lock<std::shared_mutex> lock(memoryMutex);
    return std::count(memory.begin(), memory.end(), '+');
}

size_t MemoryManager::getFreeMemory() const {
    return MEMORY_SIZE - getUsedMemory();
}

size_t MemoryManager::getExternalFragmentation() const {
    std::shared_lock<std::shared_mutex> lock(memoryMutex);

    size_t fragmentation = 0;
    size_t currentFreeBlock = 0;

    for (size_t i = 0; i < MEMORY_SIZE; ++i) {
        if (memory[i] == '.') {
            currentFreeBlock++;
        }
        else {
            if (currentFreeBlock > 0 && currentFreeBlock < 64) {
                fragmentation += currentFreeBlock;
            }
            currentFreeBlock = 0;
        }
    }

    if (currentFreeBlock > 0 && currentFreeBlock < 64) {
        fragmentation += currentFreeBlock;
    }

    return fragmentation;
}

int MemoryManager::getProcessesInMemory() const {
    std::shared_lock<std::shared_mutex> lock(memoryMutex);

    int processCount = 0;
    bool inProcess = false;

    for (size_t i = 0; i < MEMORY_SIZE; ++i) {
        if (memory[i] == '+' && !inProcess) {
            processCount++;
            inProcess = true;
        }
        else if (memory[i] == '.' && inProcess) {
            inProcess = false;
        }
    }

    return processCount;
}

void MemoryManager::printMemoryStatus() const {
    std::shared_lock<std::shared_mutex> lock(memoryMutex);

    std::cout << "\n=== Memory Manager Status ===\n";
    std::cout << "Total Memory: " << MEMORY_SIZE << " bytes\n";
    std::cout << "Used Memory: " << getUsedMemory() << " bytes\n";
    std::cout << "Free Memory: " << getFreeMemory() << " bytes\n";
    std::cout << "External Fragmentation: " << getExternalFragmentation() << " bytes\n";
    std::cout << "Processes in Memory: " << getProcessesInMemory() << "\n";

    std::cout << "\nMemory Layout (showing first 256 bytes):\n";
    for (size_t i = 0; i < std::min(static_cast<size_t>(256), MEMORY_SIZE); i += 32) {
        std::cout << std::setfill('0') << std::setw(5) << i << ": ";
        for (size_t j = i; j < std::min(i + 32, MEMORY_SIZE); ++j) {
            std::cout << memory[j];
        }
        std::cout << "\n";
    }
    std::cout << "========================\n\n";
}

void MemoryManager::reset() {
    std::unique_lock<std::shared_mutex> lock(memoryMutex);
    std::fill(memory.begin(), memory.end(), '.');
    currentCycle = 0;
}
