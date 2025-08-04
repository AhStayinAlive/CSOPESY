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
    int pageNum = address / pageSize;
    int offset = address % pageSize;
    int frameIdx = getFrame(proc, pageNum);
    return frames[frameIdx].data[offset];
}

void MemoryManager::write(std::shared_ptr<Process> proc, int address, uint8_t value) {
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
    frames[freeFrame].data = readFromBackingStore(proc->pid, virtualPage);
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
    if (fifoQueue.empty()) return;
    int victim = fifoQueue.front();
    fifoQueue.pop_front();

    int pid = frames[victim].pid;
    int pageNum = frames[victim].pageNumber;

    auto proc = ProcessManager::findByPid(pid);
    if (proc && proc->pageTable[pageNum].dirty) {
        writeToBackingStore(pid, pageNum, frames[victim].data);
        pageOuts++;
    }

    if (proc) proc->pageTable[pageNum] = { -1, false, false, 0 };
    frames[victim].occupied = false;
}

void MemoryManager::writeToBackingStore(int pid, int page, const std::vector<uint8_t>& data) {
    std::ofstream out("backing_" + std::to_string(pid) + ".bin", std::ios::binary | std::ios::in | std::ios::out);
    if (!out.is_open()) out.open("backing_" + std::to_string(pid) + ".bin", std::ios::binary | std::ios::out);
    out.seekp(page * pageSize);
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
}

std::vector<uint8_t> MemoryManager::readFromBackingStore(int pid, int page) {
    std::vector<uint8_t> buffer(pageSize, 0);
    std::ifstream in("backing_" + std::to_string(pid) + ".bin", std::ios::binary);
    if (in.is_open()) {
        in.seekg(page * pageSize);
        in.read(reinterpret_cast<char*>(buffer.data()), pageSize);
    }
    return buffer;
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