#pragma message("Compiling MemoryManager.cpp")

#include "MemoryManager.h"
#include "process.h"
#include "config.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <limits>

// Static members initialization
std::unique_ptr<MemoryManager> MemoryManager::instance;
std::once_flag MemoryManager::initFlag;
//std::mutex MemoryManager::backingStoreMutex;
//std::atomic<int> MemoryManager::currentCycle(0);
//int MemoryManager::pageFaultCount = 0;
//int MemoryManager::pageOutCount = 0;

MemoryManager::MemoryManager() {
    std::filesystem::create_directory(BACKING_STORE_DIR);
    memory.resize(MEMORY_SIZE, 0);
    memoryBlocks.clear();
    memoryBlocks.push_back({ 0, MEMORY_SIZE, true, nullptr }); // Initial free block

    size_t totalFrames = MEMORY_SIZE / Config::getInstance().memPerFrame;
    frames.resize(totalFrames);
    this->frameSize = std::max((size_t)1, Config::getInstance().memPerFrame);
}

MemoryManager& MemoryManager::getInstance() {
    std::call_once(initFlag, []() {
        instance.reset(new MemoryManager());
        });
    return *instance;
}

// Memory allocation/deallocation
bool MemoryManager::allocate(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(memLock);
    size_t req = process->getRequiredMemory();

    for (auto& block : memoryBlocks) {
        if (block.free && block.size >= req) {
            size_t oldStart = block.start;
            size_t oldSize = block.size;

            block.free = false;
            block.size = req;
            block.process = process;
            process->setBaseAddress((int)block.start);

            process->memory.resize(req, 0);

            if (oldSize > req) {
                Block newBlock{ oldStart + req, oldSize - req, true, nullptr };
                auto it = std::find(memoryBlocks.begin(), memoryBlocks.end(), block);
                memoryBlocks.insert(std::next(it), newBlock);
            }
            return true;
        }
    }
    return false;
}

void MemoryManager::deallocate(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(memLock);
    size_t base = process->getBaseAddress();

    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (!it->free && it->start == base) {
            it->free = true;
            it->process = nullptr;

            // Merge with previous block if free
            if (it != memoryBlocks.begin()) {
                auto prev = std::prev(it);
                if (prev->free) {
                    prev->size += it->size;
                    memoryBlocks.erase(it);
                    it = prev;
                }
            }

            // Merge with next block if free
            if (std::next(it) != memoryBlocks.end()) {
                auto next = std::next(it);
                if (next->free) {
                    it->size += next->size;
                    memoryBlocks.erase(next);
                }
            }
            break;
        }
    }
}

// Frame management
size_t MemoryManager::findFreeFrame() const {
    for (size_t i = 0; i < frames.size(); i++) {
        if (!frames[i].occupied) {
            return i;
        }
    }
    return INVALID_FRAME;
}

size_t MemoryManager::findLRUFrame() {
    size_t lruIndex = 0;
    int oldest = currentCycle.load();

    for (size_t i = 0; i < frames.size(); i++) {
        if (frames[i].lastUsedCycle < oldest) {
            oldest = frames[i].lastUsedCycle;
            lruIndex = i;
        }
    }
    return lruIndex;
}

void MemoryManager::loadPage(std::shared_ptr<Process> process, size_t virtualPage, size_t frameIndex) {
    frames[frameIndex].occupied = true;
    frames[frameIndex].process = process;
    frames[frameIndex].virtualPageNumber = virtualPage;
    frames[frameIndex].lastUsedCycle = currentCycle.load();
    process->pageTable[virtualPage] = frameIndex;
    process->loadedPages.insert(virtualPage);
}

void MemoryManager::evictPage(std::shared_ptr<Process> proc, size_t virtualPage) {
    if (!proc) return;
    proc->pageTable.erase(virtualPage);
    proc->loadedPages.erase(virtualPage);
}

// Page fault handling
bool MemoryManager::handlePageFault(std::shared_ptr<Process> proc, size_t virtualPage) {
    std::lock_guard<std::mutex> lock(backingStoreMutex);
    pageFaultCount++;

    // Try to find a free frame first
    size_t frameIndex = findFreeFrame();
    if (frameIndex != INVALID_FRAME) {
        loadPage(proc, virtualPage, frameIndex);
        return true;
    }

    // If no free frames, evict LRU frame
    frameIndex = findLRUFrame();
    auto victimProcess = frames[frameIndex].process.lock();
    if (victimProcess) {
        swapOutPage(frameIndex);
    }

    // Load the new page
    loadPage(proc, virtualPage, frameIndex);
    return true;
}

// Memory access
bool MemoryManager::accessMemory(const std::shared_ptr<Process>& proc, size_t virtualAddress) {
    std::lock_guard<std::mutex> lock(memLock);
    size_t page = virtualAddress / this->frameSize;

    if (!proc->pageTable.contains(page)) {
        std::string logMsg = "Memory access violation in PID " + std::to_string(proc->pid) +
            " for virtual address " + std::to_string(virtualAddress) +
            " (page " + std::to_string(page) + ")";
        std::cerr << "[ACCESS VIOLATION] " << logMsg << std::endl;
        return false;
    }

    if (!proc->loadedPages.count(page)) {
        handlePageFault(proc, page);
    }
    return true;
}

// Utility functions
void MemoryManager::incrementCycle() {
    currentCycle.fetch_add(1);
}

size_t MemoryManager::getFrameSize() const {
    return this->frameSize;
}

void MemoryManager::reset() {
    std::lock_guard<std::mutex> lock(memLock);
    memoryBlocks.clear();
    memoryBlocks.push_back({ 0, MEMORY_SIZE, true, nullptr });
    frames.assign(frames.size(), Frame());
}

// Statistics
size_t MemoryManager::getUsedMemory() const {
    size_t used = 0;
    for (const auto& block : memoryBlocks) {
        if (!block.free) used += block.size;
    }
    return used;
}

size_t MemoryManager::getFreeMemory() const {
    size_t freeMem = 0;
    for (const auto& block : memoryBlocks) {
        if (block.free) freeMem += block.size;
    }
    return freeMem;
}

void MemoryManager::printMemoryStats(const std::vector<std::shared_ptr<Process>>& allProcesses) {
    std::cout << "==== Memory Statistics ====\n"
        << "Total Frames: " << frames.size() << "\n"
        << "Used Frames: " << (frames.size() - std::count_if(frames.begin(), frames.end(),
            [](const Frame& f) { return !f.occupied; })) << "\n"
        << "Page Faults: " << pageFaultCount << "\n"
        << "Page Outs: " << pageOutCount << "\n";
}

// Add these implementations to your MemoryManager.cpp

bool MemoryManager::canAllocateAt(size_t startIndex, size_t size) const {
    std::shared_lock lock(memoryMutex);
    if (startIndex + size > MEMORY_SIZE) return false;

    // Check if the range is free
    for (const auto& block : memoryBlocks) {
        if (!block.free &&
            (block.start < startIndex + size) &&
            (block.start + block.size > startIndex)) {
            return false;
        }
    }
    return true;
}

void MemoryManager::allocateAt(size_t startIndex, size_t size) {
    std::unique_lock lock(memoryMutex);
    // Implementation would merge this with existing block management
    // Similar to your existing allocate() but at specific location
}

std::string MemoryManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    return std::format("{:%Y-%m-%d %X}", now);
}

MemoryManager::Frame* MemoryManager::getFrameForProcess(int pid, size_t virtualPage) {
    for (auto& frame : frames) {
        if (frame.occupied) {
            if (auto proc = frame.process.lock()) {
                if (proc->pid == pid && frame.virtualPageNumber == virtualPage) {
                    return &frame;
                }
            }
        }
    }
    return nullptr;
}

void MemoryManager::loadPageIntoFrame(std::shared_ptr<Process> process,
    size_t virtualPage,
    size_t frameIndex) {
    if (frameIndex >= frames.size()) return;

    auto& frame = frames[frameIndex];
    frame.occupied = true;
    frame.process = process;
    frame.virtualPageNumber = virtualPage;
    frame.lastUsedCycle = currentCycle.load();

    if (process) {
        process->pageTable[virtualPage] = frameIndex;
        process->loadedPages.insert(virtualPage);
    }
}

void MemoryManager::evictPageFromFrame(size_t frameIndex) {
    if (frameIndex >= frames.size()) return;

    auto& frame = frames[frameIndex];
    if (auto proc = frame.process.lock()) {
        proc->pageTable.erase(frame.virtualPageNumber);
        proc->loadedPages.erase(frame.virtualPageNumber);
    }

    frame = Frame(); // Reset to default
    pageOutCount++;
}

size_t MemoryManager::findEvictionCandidate() {
    size_t lruIndex = 0;
    int oldest = currentCycle.load();

    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].occupied && frames[i].lastUsedCycle < oldest) {
            oldest = frames[i].lastUsedCycle;
            lruIndex = i;
        }
    }
    return lruIndex;
}

size_t MemoryManager::getExternalFragmentation() const {
    std::shared_lock lock(memoryMutex);
    size_t fragmentation = 0;

    // Sum all free blocks that are too small to be useful (smaller than smallest allocation unit)
    const size_t minAllocationSize = this->frameSize; // Could also be a fixed minimum size
    for (const auto& block : memoryBlocks) {
        if (block.free && block.size >= minAllocationSize) {
            fragmentation += block.size;
        }
    }

    return fragmentation;
}

int MemoryManager::getProcessesInMemory() const {
    std::shared_lock lock(memoryMutex);
    int count = 0;

    // Count all non-free blocks with active processes
    for (const auto& block : memoryBlocks) {
        if (!block.free && block.process) {
            count++;
        }
    }

    return count;
}

void MemoryManager::printMemoryStatus() const {
    std::shared_lock lock(memoryMutex);
    std::cout << "\n=== Memory Status ===" << std::endl;
    std::cout << "Total Memory: " << MEMORY_SIZE << " bytes" << std::endl;
    std::cout << "Used Memory: " << getUsedMemory() << " bytes" << std::endl;
    std::cout << "Free Memory: " << getFreeMemory() << " bytes" << std::endl;
    std::cout << "External Fragmentation: " << getExternalFragmentation() << " bytes" << std::endl;
    std::cout << "Processes in Memory: " << getProcessesInMemory() << std::endl;
    std::cout << "Frames: " << getUsedFrames() << "/" << getTotalFrames() << " used" << std::endl;
    std::cout << "Page Faults: " << pageFaultCount.load() << std::endl;
    std::cout << "====================" << std::endl;
}

size_t MemoryManager::getUsedFrames() const {
    std::shared_lock lock(memoryMutex);
    return std::count_if(frames.begin(), frames.end(),
        [](const Frame& f) { return f.occupied; });
}

size_t MemoryManager::getTotalFrames() const {
    return frames.size();
}

void MemoryManager::writeToBackingStore(const std::shared_ptr<Process>& proc,
    size_t pageNumber,
    const std::string& varName,
    int value) {
    std::lock_guard lock(backingStoreMutex);

    // Create directory if it doesn't exist
    std::filesystem::create_directory(BACKING_STORE_DIR);

    // Write to backing store file
    std::ofstream outFile(BACKING_STORE_DIR + "/" + BACKING_STORE_FILE,
        std::ios::binary | std::ios::app);
    if (!outFile) {
        std::cerr << "Failed to open backing store file for writing" << std::endl;
        return;
    }

    // Write process ID, page number, variable name and value
    outFile.write(reinterpret_cast<const char*>(&proc->pid), sizeof(proc->pid));
    outFile.write(reinterpret_cast<const char*>(&pageNumber), sizeof(pageNumber));
    outFile.write(varName.c_str(), varName.size() + 1); // +1 for null terminator
    outFile.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

bool MemoryManager::loadFromBackingStore(const std::shared_ptr<Process>& proc,
    size_t pageNumber,
    std::string varName,
    int& value) {
    std::lock_guard lock(backingStoreMutex);

    std::ifstream inFile(BACKING_STORE_DIR + "/" + BACKING_STORE_FILE,
        std::ios::binary);
    if (!inFile) {
        return false;
    }

    while (inFile) {
        int storedPid;
        size_t storedPage;

        // Read PID and page number
        inFile.read(reinterpret_cast<char*>(&storedPid), sizeof(storedPid));
        inFile.read(reinterpret_cast<char*>(&storedPage), sizeof(storedPage));

        // Check if this is the record we're looking for
        if (storedPid == proc->pid && storedPage == pageNumber) {
            // Read variable name
            std::string storedVarName;
            std::getline(inFile, storedVarName, '\0');

            // Check variable name match
            if (storedVarName == varName) {
                // Read the value
                inFile.read(reinterpret_cast<char*>(&value), sizeof(value));
                return true;
            }
            else {
                // Skip the value to continue searching
                inFile.seekg(sizeof(value), std::ios::cur);
            }
        }
        else {
            // Skip to next record
            // Skip variable name (read until null terminator)
            while (inFile.get() != 0 && inFile);
            // Skip value
            inFile.seekg(sizeof(value), std::ios::cur);
        }
    }

    return false;
}

void MemoryManager::swapOutPage(size_t frameIndex) {
    if (frameIndex >= frames.size()) return;

    std::lock_guard<std::mutex> lock(backingStoreMutex);
    Frame& frame = frames[frameIndex];

    if (!frame.occupied) return;

    // Write to backing store
    std::ofstream outFile(BACKING_STORE_DIR + "/" + BACKING_STORE_FILE,
        std::ios::binary | std::ios::app);
    if (outFile) {
        // Write process ID, virtual page number, and variables
        if (auto proc = frame.process.lock()) {
            outFile.write(reinterpret_cast<const char*>(&proc->pid), sizeof(proc->pid));
            outFile.write(reinterpret_cast<const char*>(&frame.virtualPageNumber),
                sizeof(frame.virtualPageNumber));

            for (const auto& [varName, value] : frame.variables) {
                outFile.write(varName.c_str(), varName.size() + 1); // +1 for null terminator
                outFile.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
    }

    // Clear the frame
    frame.occupied = false;
    frame.process.reset();
    frame.variables.clear();
    frame.virtualPageNumber = 0;
    frame.lastUsedCycle = 0;

    pageOutCount++;
}

void MemoryManager::swapInPage(std::shared_ptr<Process> proc,
    size_t virtualPage,
    size_t frameIndex) {
    if (frameIndex >= frames.size()) return;

    std::lock_guard<std::mutex> lock(backingStoreMutex);
    Frame& frame = frames[frameIndex];

    // 1. Try to load from backing store
    bool loadedFromStore = false;
    std::ifstream inFile(BACKING_STORE_DIR + "/" + BACKING_STORE_FILE, std::ios::binary);

    while (inFile) {
        int storedPid;
        size_t storedPage;

        inFile.read(reinterpret_cast<char*>(&storedPid), sizeof(storedPid));
        inFile.read(reinterpret_cast<char*>(&storedPage), sizeof(storedPage));

        if (storedPid == proc->pid && storedPage == virtualPage) {
            // Found our page - load variables
            while (inFile.peek() != EOF) {
                std::string varName;
                std::getline(inFile, varName, '\0');
                uint16_t value;
                inFile.read(reinterpret_cast<char*>(&value), sizeof(value));

                // Find the memory address for the variable
                if (proc->variableAddressMap.find(varName) != proc->variableAddressMap.end()) {
                    size_t addr = proc->variableAddressMap[varName];
                    proc->writeMemory(addr, value); // Use helper for writing to raw memory
                }

                frame.variables[varName] = value; // Still store for frame-level tracking (optional)
            }

            loadedFromStore = true;
            break;
        }

        // Skip to next record
        inFile.ignore(std::numeric_limits<std::streamsize>::max(), '\0');
        inFile.ignore(sizeof(uint16_t));
    }

    // 2. Initialize frame metadata
    frame.occupied = true;
    frame.process = proc;
    frame.virtualPageNumber = virtualPage;
    frame.lastUsedCycle = currentCycle.load();

    // 3. Update process page table
    proc->pageTable[virtualPage] = frameIndex;
    proc->loadedPages.insert(virtualPage);
}


bool MemoryManager::isPageInMemory(std::shared_ptr<Process> proc, size_t virtualPage) {
    if (!proc) return false;
    return proc->loadedPages.count(virtualPage) > 0;
}

void MemoryManager::ensurePageLoaded(std::shared_ptr<Process> proc, size_t virtualAddress) {
    if (!proc) return;

    size_t virtualPage = virtualAddress / this->frameSize;

    if (!isPageInMemory(proc, virtualPage)) {
        handlePageFault(proc, virtualPage);
    }

    // Update last used cycle
    if (auto frame = getFrameForProcess(proc->pid, virtualPage)) {
        frame->lastUsedCycle = currentCycle.load();
    }
}

uint16_t MemoryManager::read(std::shared_ptr<Process> proc, size_t virtualAddress) {
    ensurePageLoaded(proc, virtualAddress);

    size_t virtualPage = virtualAddress / this->frameSize;
    size_t offset = virtualAddress % this->frameSize;

    if (!proc->pageTable.contains(virtualPage)) {
        throw std::runtime_error("Page not mapped for read at address " + std::to_string(virtualAddress));
    }

    size_t frameIndex = proc->pageTable[virtualPage];
    size_t physicalAddress = frameIndex * this->frameSize + offset;

    if (physicalAddress + 1 >= memory.size()) {
        throw std::runtime_error("Read overflow at physical address " + std::to_string(physicalAddress));
    }

    uint16_t value = (static_cast<uint16_t>(this->memory[physicalAddress +1]) << 8) |
        static_cast<uint8_t>(this->memory[physicalAddress]);

    return value;
}

void MemoryManager::write(std::shared_ptr<Process> proc, size_t virtualAddress, uint16_t value) {
    ensurePageLoaded(proc, virtualAddress);

    size_t virtualPage = virtualAddress / this->frameSize;
    size_t offset = virtualAddress % this->frameSize;

    if (!proc->pageTable.contains(virtualPage)) {
        throw std::runtime_error("Page not mapped for write at address " + std::to_string(virtualAddress));
    }

    size_t frameIndex = proc->pageTable[virtualPage];
    size_t physicalAddress = frameIndex * this->frameSize + offset;

    if (physicalAddress + 1 >= memory.size()) {
        throw std::runtime_error("Write overflow at physical address " + std::to_string(physicalAddress));
    }

    this->memory[physicalAddress] = static_cast<uint8_t>(value & 0xFF);
    this->memory[physicalAddress + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}