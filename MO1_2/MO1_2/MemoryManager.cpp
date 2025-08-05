#include "MemoryManager.h"
#include "ProcessManager.h"
#include "config.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <algorithm>

MemoryManager::MemoryManager() {}

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

void MemoryManager::initialize() {
    Config& config = Config::getInstance();
    pageSize = config.memPerFrame;
    totalFrames = config.maxOverallMem / pageSize;
    frames.resize(totalFrames);

    for (auto& frame : frames) {
        frame.occupied = false;
        frame.pid = -1;
        frame.pageNumber = -1;
        frame.data.resize(pageSize, 0); // Initialize with zeros
    }

    // Clear any existing backing store file
    std::ofstream backingStore("csopesy-backing-store.txt", std::ios::trunc);
    backingStore << "# CSOPESY Backing Store - Format: PID PAGE_NUM DATA_BYTES\n";
    backingStore.close();
}

uint8_t MemoryManager::read(std::shared_ptr<Process> proc, int address) {
    if (address < 0) {
        throw std::runtime_error("Invalid negative address: " + std::to_string(address));
    }

    // ✅ FIXED: Dynamic virtual memory expansion
    if (address >= proc->virtualMemoryLimit) {
        // Expand virtual memory to accommodate the address
        int newLimit = ((address / 512) + 1) * 512;  // Expand in 512-byte chunks
        proc->virtualMemoryLimit = newLimit;

        std::ostringstream log;
        log << "[MEMORY] Expanded virtual memory limit to " << newLimit
            << " bytes to access address 0x" << std::hex << std::uppercase << address;
        proc->logs.push_back(log.str());
    }

    int pageNum = address / pageSize;
    int offset = address % pageSize;
    int frameIdx = getFrame(proc, pageNum);

    if (frameIdx == INVALID_FRAME) {
        std::ostringstream oss;
        oss << "Memory access violation: failed to load page " << pageNum
            << " for address 0x" << std::hex << std::uppercase << address;
        throw std::runtime_error(oss.str());
    }

    return frames[frameIdx].data[offset];
}

void MemoryManager::write(std::shared_ptr<Process> proc, int address, uint8_t value) {
    if (address < 0) {
        throw std::runtime_error("Invalid negative address: " + std::to_string(address));
    }

    // ✅ FIXED: Dynamic virtual memory expansion
    if (address >= proc->virtualMemoryLimit) {
        // Expand virtual memory to accommodate the address
        int newLimit = ((address / 512) + 1) * 512;  // Expand in 512-byte chunks
        proc->virtualMemoryLimit = newLimit;

        std::ostringstream log;
        log << "[MEMORY] Expanded virtual memory limit to " << newLimit
            << " bytes to access address 0x" << std::hex << std::uppercase << address;
        proc->logs.push_back(log.str());
    }

    int pageNum = address / pageSize;
    int offset = address % pageSize;
    int frameIdx = getFrame(proc, pageNum);

    if (frameIdx == INVALID_FRAME) {
        std::ostringstream oss;
        oss << "Memory access violation: failed to load page " << pageNum
            << " for address 0x" << std::hex << std::uppercase << address;
        throw std::runtime_error(oss.str());
    }

    frames[frameIdx].data[offset] = value;
    proc->pageTable[pageNum].dirty = true;
}

// ✅ CRITICAL: This function was missing - here's the implementation
int MemoryManager::getFrame(std::shared_ptr<Process> proc, int virtualPage) {
    auto it = proc->pageTable.find(virtualPage);
    if (it != proc->pageTable.end() && it->second.valid) {
        return it->second.frameNumber;
    }
    return loadPage(proc, virtualPage);
}

int MemoryManager::loadPage(std::shared_ptr<Process> proc, int virtualPage) {
    int freeFrame = findFreeFrame();

    // If no free frame, evict one
    if (freeFrame == INVALID_FRAME) {
        evictPage();
        freeFrame = findFreeFrame();

        if (freeFrame == INVALID_FRAME) {
            throw std::runtime_error("Critical: No memory available after eviction");
        }
    }

    // Load page into the free frame
    frames[freeFrame].pid = proc->pid;
    frames[freeFrame].pageNumber = virtualPage;
    frames[freeFrame].occupied = true;

    // Try to restore data from backing store
    auto restoredData = readFromBackingStore(proc->pid, virtualPage);
    if (!restoredData.empty()) {
        if (restoredData.size() != pageSize) {
            restoredData.resize(pageSize, 0);
        }
        frames[freeFrame].data = restoredData;
    }
    else {
        // Initialize with zeros if not in backing store
        std::fill(frames[freeFrame].data.begin(), frames[freeFrame].data.end(), 0);
    }

    pageIns++;

    // Update page table
    proc->pageTable[virtualPage] = { freeFrame, true, false, 0 };
    fifoQueue.push_back(freeFrame);

    return freeFrame;
}

int MemoryManager::findFreeFrame() {
    for (int i = 0; i < totalFrames; ++i) {
        if (!frames[i].occupied) {
            return i;
        }
    }
    return INVALID_FRAME;
}

void MemoryManager::evictPage() {
    if (fifoQueue.empty()) {
        // Emergency: find any occupied frame to evict
        for (int i = 0; i < totalFrames; ++i) {
            if (frames[i].occupied) {
                fifoQueue.push_back(i);
                break;
            }
        }
        if (fifoQueue.empty()) {
            return; // No frames to evict
        }
    }

    int victim = fifoQueue.front();
    fifoQueue.pop_front();

    int pid = frames[victim].pid;
    int pageNum = frames[victim].pageNumber;

    auto proc = ProcessManager::findByPid(pid);
    if (proc && proc->pageTable.count(pageNum) && proc->pageTable[pageNum].dirty) {
        // Write dirty page to backing store
        writeToBackingStore(pid, pageNum, frames[victim].data);
        pageOuts++;
    }

    // Update page table if process still exists
    if (proc && proc->pageTable.count(pageNum)) {
        proc->pageTable[pageNum] = { -1, false, false, 0 };
    }

    // Free the frame
    frames[victim].occupied = false;
    frames[victim].pid = -1;
    frames[victim].pageNumber = -1;
    std::fill(frames[victim].data.begin(), frames[victim].data.end(), 0);
}

void MemoryManager::writeToBackingStore(int pid, int page, const std::vector<uint8_t>& data) {
    // First, remove any existing entry for this PID+PAGE combination
    removeFromBackingStore(pid, page);

    // Append new entry
    std::ofstream file("csopesy-backing-store.txt", std::ios::app);
    if (!file) {
        std::cerr << "[ERROR] Cannot open backing store for writing\n";
        return;
    }

    file << pid << " " << page << " ";
    for (size_t i = 0; i < data.size(); ++i) {
        file << static_cast<int>(data[i]);
        if (i < data.size() - 1) file << " ";
    }
    file << "\n";
    file.close();
}

std::vector<uint8_t> MemoryManager::readFromBackingStore(int pid, int page) {
    std::ifstream file("csopesy-backing-store.txt");
    if (!file) {
        return {}; // File doesn't exist or can't be opened
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip comments and empty lines

        std::istringstream iss(line);
        int foundPid, foundPage;

        if (!(iss >> foundPid >> foundPage)) continue;

        if (foundPid == pid && foundPage == page) {
            std::vector<uint8_t> data;
            int byte;
            while (iss >> byte) {
                data.push_back(static_cast<uint8_t>(byte));
            }
            file.close();
            return data;
        }
    }

    file.close();
    return {}; // Not found
}

void MemoryManager::removeFromBackingStore(int pid, int page) {
    std::ifstream inFile("csopesy-backing-store.txt");
    if (!inFile) return;

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') {
            lines.push_back(line);
            continue;
        }

        std::istringstream iss(line);
        int foundPid, foundPage;

        if (!(iss >> foundPid >> foundPage)) {
            lines.push_back(line);
            continue;
        }

        // Only keep lines that don't match our PID+PAGE
        if (!(foundPid == pid && foundPage == page)) {
            lines.push_back(line);
        }
    }
    inFile.close();

    // Write back the filtered content
    std::ofstream outFile("csopesy-backing-store.txt", std::ios::trunc);
    for (const auto& l : lines) {
        outFile << l << "\n";
    }
    outFile.close();
}

bool MemoryManager::hasFreeFrame() {
    return findFreeFrame() != INVALID_FRAME;
}

bool MemoryManager::isFrameOccupied(int index) const {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        return frames[index].occupied;
    }
    return false;
}

int MemoryManager::allocateVariable(std::shared_ptr<Process> proc, const std::string& varName) {
    // Check if variable already exists
    if (proc->variableTable.count(varName)) {
        return proc->variableTable[varName];
    }

    // ✅ OPTION 2: Dynamically expand virtual memory limit if needed
    if (proc->nextFreeAddress >= proc->virtualMemoryLimit) {
        // Expand virtual memory in chunks (e.g., double it)
        proc->virtualMemoryLimit *= 2;
        
        std::ostringstream log;
        log << "Expanded virtual memory limit to " << proc->virtualMemoryLimit << " bytes";
        proc->logs.push_back(log.str());
    }

    int address = proc->nextFreeAddress;
    proc->variableTable[varName] = address;
    proc->nextFreeAddress += sizeof(uint16_t);

    return address;
}

void MemoryManager::freeProcessMemory(int pid) {
    // Free all frames used by this process
    int freedFrames = 0;
    for (int i = 0; i < totalFrames; ++i) {
        if (frames[i].occupied && frames[i].pid == pid) {
            frames[i].occupied = false;
            frames[i].pid = -1;
            frames[i].pageNumber = -1;
            std::fill(frames[i].data.begin(), frames[i].data.end(), 0);
            freedFrames++;
        }
    }

    // Remove from FIFO queue
    fifoQueue.erase(
        std::remove_if(fifoQueue.begin(), fifoQueue.end(),
            [pid, this](int frameIdx) {
                return !frames[frameIdx].occupied || frames[frameIdx].pid == pid;
            }),
        fifoQueue.end());

    // Clean up backing store entries for this process
    cleanupBackingStore(pid);
}

void MemoryManager::cleanupBackingStore(int pid) {
    std::ifstream inFile("csopesy-backing-store.txt");
    if (!inFile) return;

    std::vector<std::string> lines;
    std::string line;
    int removedEntries = 0;

    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') {
            lines.push_back(line);
            continue;
        }

        std::istringstream iss(line);
        int foundPid;

        if (!(iss >> foundPid)) {
            lines.push_back(line);
            continue;
        }

        // Only keep lines that don't match our PID
        if (foundPid != pid) {
            lines.push_back(line);
        }
        else {
            removedEntries++;
        }
    }
    inFile.close();

    // Write back the filtered content
    std::ofstream outFile("csopesy-backing-store.txt", std::ios::trunc);
    for (const auto& l : lines) {
        outFile << l << "\n";
    }
    outFile.close();
}

int MemoryManager::getUsedFrames() const {
    int count = 0;
    for (const auto& frame : frames) {
        if (frame.occupied) count++;
    }
    return count;
}

void MemoryManager::printMemoryStatus() const {
    std::cout << "=== MEMORY MANAGER STATUS ===\n";
    std::cout << "Total Frames: " << totalFrames << "\n";
    std::cout << "Used Frames: " << getUsedFrames() << "\n";
    std::cout << "Free Frames: " << (totalFrames - getUsedFrames()) << "\n";
    std::cout << "Page Size: " << pageSize << " bytes\n";
    std::cout << "Page Ins: " << pageIns << "\n";
    std::cout << "Page Outs: " << pageOuts << "\n";
    std::cout << "FIFO Queue Size: " << fifoQueue.size() << "\n";
    std::cout << "=============================\n";
}