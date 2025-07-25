#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <shared_mutex>
#include <memory>
#include <string>
#include <atomic>

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
    MemoryManager();

    // Internal helpers
    bool canAllocateAt(size_t startIndex, size_t size) const;
    void allocateAt(size_t startIndex, size_t size);
    std::string getCurrentTimestamp() const;

public:
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

    size_t getTotalMemory() const { return MEMORY_SIZE; }

    // Disable copy/move
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
};

#endif // MEMORY_MANAGER_H
