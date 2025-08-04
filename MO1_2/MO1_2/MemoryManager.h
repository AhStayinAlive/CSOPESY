// MemoryManager.h - Updated with proper memory management
#pragma once

#include <unordered_map>
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include "process.h"
#include "config.h"

constexpr int INVALID_FRAME = -1;

struct Frame {
    int pid = -1;
    int pageNumber = -1;
    bool occupied = false;
    std::vector<uint8_t> data;
};

class MemoryManager {
public:
    void initialize();
    uint8_t read(std::shared_ptr<Process> proc, int address);
    void write(std::shared_ptr<Process> proc, int address, uint8_t value);

    static MemoryManager& getInstance();
    bool hasFreeFrame();
    bool isFrameOccupied(int index) const;

    int getPageIns() const { return pageIns; }
    int getPageOuts() const { return pageOuts; }
    int allocateVariable(std::shared_ptr<Process> proc, const std::string& varName);

    // New methods for proper memory management
    void freeProcessMemory(int pid);
    void cleanupBackingStore(int pid);
    int getTotalFrames() const { return totalFrames; }
    int getUsedFrames() const;
    void printMemoryStatus() const;

private:
    MemoryManager();

    int getFrame(std::shared_ptr<Process> proc, int virtualPage);
    int loadPage(std::shared_ptr<Process> proc, int virtualPage);
    void evictPage();
    int findFreeFrame();

    // Backing store methods
    void writeToBackingStore(int pid, int virtualPage, const std::vector<uint8_t>& data);
    std::vector<uint8_t> readFromBackingStore(int pid, int virtualPage);
    void removeFromBackingStore(int pid, int page);

    int pageSize = 0;
    int totalFrames = 0;
    int pageIns = 0;
    int pageOuts = 0;

    std::vector<Frame> frames;
    std::deque<int> fifoQueue; // stores frame indexes in FIFO order
};