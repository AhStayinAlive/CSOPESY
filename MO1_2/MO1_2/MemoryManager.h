#pragma once
#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <vector>
#include <shared_mutex>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>

// Forward declare to avoid cyclic include
class Process;

class MemoryManager {
private:
    struct Block {
        size_t start;
        size_t size;
        bool free;
        std::shared_ptr<Process> process;

        bool operator==(const Block& other) const {
            return start == other.start && size == other.size && free == other.free &&
                process == other.process;
        }

    };

    std::vector<Block> memoryBlocks;

    static constexpr size_t MEMORY_SIZE = 16384; // 16KB memory
    static std::unique_ptr<MemoryManager> instance;
    static std::once_flag initFlag;

    // Memory management
    std::vector<char> memory;
    mutable std::shared_mutex memoryMutex;
    mutable std::mutex memLock;
    size_t frameSize;

    // Backing store
    const std::string BACKING_STORE_DIR = "csopesy-backing-store";
    const std::string BACKING_STORE_FILE = "page_store.bin";
    std::unordered_map<std::shared_ptr<Process>, std::vector<size_t>> backingStore;

    // Frame management
    std::unordered_map<size_t, size_t> frameAccessCycle;  // Tracks last access cycles
    static const size_t INVALID_FRAME = static_cast<size_t>(-1);

    // Statistics
    static inline std::atomic<int> currentCycle{ 0 };
    static inline std::mutex backingStoreMutex;
    static inline std::atomic<int> pageFaultCount{ 0 };
    static inline std::atomic<int> pageOutCount{ 0 };

    // Private constructor
    MemoryManager();

    // Internal helpers
    bool canAllocateAt(size_t startIndex, size_t size) const;
    void allocateAt(size_t startIndex, size_t size);
    std::string getCurrentTimestamp() const;
    size_t findFreeFrame() const;
    size_t findLRUFrame();

public:
    // Singleton accessor
    static MemoryManager& getInstance();

    struct Frame {
        bool occupied = false;
        std::weak_ptr<Process> process; // Use weak_ptr to avoid circular references
        size_t virtualPageNumber = 0;
        int lastUsedCycle = 0; // For LRU
        std::unordered_map<std::string, uint16_t> variables;

        Frame() : occupied(false), virtualPageNumber(0), lastUsedCycle(0) {}
    };

    std::vector<Frame> frames;

    // Memory operations
    bool allocate(std::shared_ptr<Process> process);
    void deallocate(std::shared_ptr<Process> process);
    void visualizeMemory(int cycle);
    void incrementCycle();
    void reset();

    // Page management
    bool handlePageFault(std::shared_ptr<Process> proc, size_t virtualPage);
    void swapOutPage(size_t frameIndex);
    void swapInPage(std::shared_ptr<Process> proc, size_t virtualPage, size_t frameIndex);
    bool isPageInMemory(std::shared_ptr<Process> proc, size_t virtualPage);
    void ensurePageLoaded(std::shared_ptr<Process> proc, size_t virtualAddress);
    bool accessMemory(const std::shared_ptr<Process>& proc, size_t virtualAddress);

    // Frame operations
    Frame* getFrameForProcess(int pid, size_t virtualPage);
    void loadPage(std::shared_ptr<Process> process, size_t virtualPage, size_t frameIndex);
    void evictPage(std::shared_ptr<Process> proc, size_t virtualPage);
    void loadPageIntoFrame(std::shared_ptr<Process> process, size_t virtualPage, size_t frameIndex);
    void evictPageFromFrame(size_t frameIndex);
    size_t findEvictionCandidate();

    // Backing store operations
    void writeToBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber,
        const std::string& varName, int value);
    bool loadFromBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber,
        std::string varName, int& value);

    // Statistics and info
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    size_t getExternalFragmentation() const;
    int getProcessesInMemory() const;
    void printMemoryStatus() const;
    size_t getUsedFrames() const;
    size_t getTotalFrames() const;
    size_t getTotalMemory() const { return MEMORY_SIZE; }
    size_t getFrameSize() const;
    void printMemoryStats(const std::vector<std::shared_ptr<Process>>& allProcesses);
    int getPageFaultCount() const { return pageFaultCount.load(); }
    int getPageOutCount() const { return pageOutCount.load(); }

    // Disable copy/move
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;
};
#endif // MEMORYMANAGER_H