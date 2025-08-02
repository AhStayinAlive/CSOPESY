#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#pragma once
#include <vector>
#include <shared_mutex>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>

// ✅ Forward declare to avoid cyclic include
struct Process;

class MemoryManager {
private:
    static constexpr size_t MEMORY_SIZE = 16384;
    static std::unique_ptr<MemoryManager> instance;
    static std::once_flag initFlag;

    std::vector<char> memory;
    mutable std::shared_mutex memoryMutex;
    std::atomic<int> currentCycle{ 0 };

    // Constructor
    //MemoryManager();

    // Internal helpers
    bool canAllocateAt(size_t startIndex, size_t size) const;
    void allocateAt(size_t startIndex, size_t size);
    std::string getCurrentTimestamp() const;

    struct Frame {
        bool occupied = false;
        std::shared_ptr<Process> process;
        size_t virtualPageNumber = 0;
        int lastUsedCycle = 0; // For LRU
    };

    std::vector<Frame> frames;
    std::unordered_map<std::shared_ptr<Process>, std::vector<size_t>> backingStore;
    void loadPage(std::shared_ptr<Process> process, size_t virtualPage, size_t frameIndex);
    void evictPage(std::shared_ptr<Process> proc, size_t virtualPage);
    size_t frameSize;

public:
    MemoryManager();
    // Singleton accessor
    static MemoryManager& getInstance();

    // Memory ops
    bool allocate(std::shared_ptr<Process> process);
    void deallocate(std::shared_ptr<Process> process);
    void visualizeMemory(int cycle);
    void incrementCycle();
    void reset();

    // Stats
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    size_t getExternalFragmentation() const;
    int getProcessesInMemory() const;
    void printMemoryStatus() const;

    size_t getUsedFrames() const;
    size_t getTotalFrames() const;

    size_t getTotalMemory() const { return MEMORY_SIZE; }

    // Disable copy/move
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Inside public:
    bool handlePageFault(std::shared_ptr<Process> process, size_t virtualPageNumber);
    bool isPageInMemory(std::shared_ptr<Process> proc, size_t virtualPage);
    void ensurePageLoaded(std::shared_ptr<Process> proc, size_t virtualAddress);

    size_t getFrameSize() const;  // returns the frame size (e.g., 4096 bytes)
    bool accessMemory(const std::shared_ptr<Process>& proc, size_t virtualAddress);
    void printMemoryStats(const std::vector<std::shared_ptr<Process>>& allProcesses);
    void writeToBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber, const std::string& varName, int value);
    bool loadFromBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber, std::string& varName, int& value);

};

#endif // MEMORY_MANAGER_H
