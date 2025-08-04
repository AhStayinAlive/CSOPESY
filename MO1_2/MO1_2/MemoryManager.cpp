// MemoryManager.cpp
#include "MemoryManager.h"
#include "ProcessManager.h"
#include "config.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <sstream>

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
        frame.data.resize(pageSize);
    }
}

uint8_t MemoryManager::read(std::shared_ptr<Process> proc, int address) {
    if (address < 0 || address >= proc->virtualMemoryLimit) {
        throw std::runtime_error("Memory access violation: address out of bounds.");
    }
    int pageNum = address / pageSize;
    int offset = address % pageSize;
    int frameIdx = getFrame(proc, pageNum);
    return frames[frameIdx].data[offset];
}

void MemoryManager::write(std::shared_ptr<Process> proc, int address, uint8_t value) {
    if (address < 0 || address >= proc->virtualMemoryLimit) {
        throw std::runtime_error("Memory access violation: address out of bounds.");
    }
    int pageNum = address / pageSize;
    int offset = address % pageSize;
    int frameIdx = getFrame(proc, pageNum);
    frames[frameIdx].data[offset] = value;
    proc->pageTable[pageNum].dirty = true;
}

int MemoryManager::getFrame(std::shared_ptr<Process> proc, int virtualPage) {
    auto it = proc->pageTable.find(virtualPage);
    if (it != proc->pageTable.end() && it->second.valid) {
        return it->second.frameNumber;
    }
    return loadPage(proc, virtualPage);
}

int MemoryManager::loadPage(std::shared_ptr<Process> proc, int virtualPage) {
    int freeFrame = INVALID_FRAME;
    for (int i = 0; i < totalFrames; ++i) {
        if (!frames[i].occupied) {
            freeFrame = i;
            break;
        }
    }

    while (freeFrame == INVALID_FRAME) {
        evictPage();

        for (int i = 0; i < totalFrames; ++i) {
            if (!frames[i].occupied) {
                freeFrame = i;
                break;
            }
        }
    }

    frames[freeFrame].pid = proc->pid;
    frames[freeFrame].pageNumber = virtualPage;
    frames[freeFrame].occupied = true;
    auto restored = readFromBackingStore(proc->pid, virtualPage);
    if (restored.size() < pageSize) {
        restored.resize(pageSize, 0); // pad with zeros to avoid underrun
    }
    frames[freeFrame].data = restored;
    pageIns++;

    proc->pageTable[virtualPage] = { freeFrame, true, false, 0 };
    fifoQueue.push_back(freeFrame);

    std::ostringstream ss;
    ss << "[PF] Loaded page " << virtualPage << " of PID " << proc->pid << " into frame " << freeFrame;
    proc->logs.push_back(ss.str());
    logToFile(proc->name, ss.str(), proc->coreAssigned);
    return freeFrame;
}

void MemoryManager::evictPage() {
    std::cout << "[DEBUG] evictPage() called.\n";
    if (fifoQueue.empty()) return;
    int victim = fifoQueue.front();
    fifoQueue.pop_front();

    int pid = frames[victim].pid;
    int pageNum = frames[victim].pageNumber;

    auto proc = ProcessManager::findByPid(pid);
    if (proc && proc->pageTable[pageNum].dirty) {
        auto data = frames[victim].data;
        if (data.size() < pageSize) data.resize(pageSize, 0);

        std::cout << "[WRITE] Evicting PID=" << pid << " PAGE=" << pageNum << "\n";

        writeToBackingStore(pid, pageNum, data);
        pageOuts++;

    }

    if (proc) proc->pageTable[pageNum] = { -1, false, false, 0 };
    frames[victim].occupied = false;
}

void MemoryManager::writeToBackingStore(int pid, int page, const std::vector<uint8_t>& data) {
    std::ofstream file("csopesy-backing-store.txt", std::ios::app); // append mode
    if (!file) {
        std::cerr << "Error opening backing store for writing.\n";
        return;
    }

    file << "PID=" << pid << " PAGE=" << page << " DATA=";
    for (uint8_t byte : data) {
        file << static_cast<int>(byte) << " ";
    }
    file << "\n";
    file.close();

    std::cout << "[BACKING STORE] Wrote PID=" << pid << " PAGE=" << page << " (" << data.size() << " bytes)\n";

}


std::vector<uint8_t> MemoryManager::readFromBackingStore(int pid, int page) {
    std::ifstream file("csopesy-backing-store.txt");
    if (!file) {
        std::cerr << "Error opening backing store for reading.\n";
        return {};
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string pidStr, pageStr, dataLabel;
        int foundPid, foundPage;

        iss >> pidStr >> pageStr >> dataLabel;

        if (sscanf_s(pidStr.c_str(), "PID=%d", &foundPid) != 1) continue;
        if (sscanf_s(pageStr.c_str(), "PAGE=%d", &foundPage) != 1) continue;

        if (foundPid == pid && foundPage == page) {
            std::vector<uint8_t> data;
            int byte;
            while (iss >> byte) {
                data.push_back(static_cast<uint8_t>(byte));
            }
            return data;
        }
    }

    //std::cerr << "Page not found in backing store: PID=" << pid << " PAGE=" << page << "\n";
    return {};
}


bool MemoryManager::hasFreeFrame() {
    for (const auto& frame : frames) {
        if (!frame.occupied) return true;
    }
    return false;
}

bool MemoryManager::isFrameOccupied(int index) const {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        return frames[index].occupied;
    }
    return false;
}

int MemoryManager::allocateVariable(std::shared_ptr<Process> proc, const std::string& varName) {
    if (proc->variableTable.count(varName)) {
        return proc->variableTable[varName];  // already exists
    }

    if (proc->nextFreeAddress + 1 > proc->virtualMemoryLimit) {
        throw std::runtime_error("Out of memory: cannot allocate variable '" + varName + "'");
    }

    int address = proc->nextFreeAddress;
    proc->variableTable[varName] = address;
    proc->nextFreeAddress += 1;

    return address;
}
