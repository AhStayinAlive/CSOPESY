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

// Static members
std::unique_ptr<MemoryManager> MemoryManager::instance;
std::once_flag MemoryManager::initFlag;

// Block structure for internal tracking
struct Block {
    size_t start;
    size_t size;
    bool free;
    std::shared_ptr<Process> process;
};

static std::vector<Block> memoryBlocks;
static std::mutex memLock;

MemoryManager::MemoryManager() {
    memory.resize(MEMORY_SIZE, 0);
    memoryBlocks.clear();
    // Initially, one big free block
    memoryBlocks.push_back({ 0, MEMORY_SIZE, true, nullptr });

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

bool MemoryManager::allocate(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(memLock);
    size_t req = process->getRequiredMemory();

    for (auto& block : memoryBlocks) {
        if (block.free && block.size >= req) {
            // Allocate here
            size_t oldStart = block.start;
            size_t oldSize = block.size;

            block.free = false;
            block.size = req;
            block.process = process;
            process->setBaseAddress((int)block.start);

            // If block is larger than needed, split it
            if (oldSize > req) {
                Block newBlock;
                newBlock.start = oldStart + req;
                newBlock.size = oldSize - req;
                newBlock.free = true;
                newBlock.process = nullptr;

                auto it = std::find_if(memoryBlocks.begin(), memoryBlocks.end(), [&](const Block& b) {
                    return b.start == oldStart;
                    });
                if (it != memoryBlocks.end()) {
                    memoryBlocks.insert(std::next(it), newBlock);
                }
            }
            return true;
        }
    }


    return false; // No space found
}

void MemoryManager::deallocate(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(memLock);
    size_t base = process->getBaseAddress();

    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        if (!it->free && it->start == base) {
            it->free = true;
            it->process = nullptr;

            // Merge adjacent free blocks
            if (it != memoryBlocks.begin()) {
                auto prev = std::prev(it);
                if (prev->free) {
                    prev->size += it->size;
                    memoryBlocks.erase(it);
                    it = prev;
                }
            }
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

void MemoryManager::incrementCycle() {
    int cycle = currentCycle.fetch_add(1) + 1;
    visualizeMemory(cycle);
}

void MemoryManager::visualizeMemory(int cycle) {
    std::lock_guard<std::mutex> lock(memLock);

    std::ostringstream filename;
    filename << "memory_stamp_" << cycle << ".txt";
    std::ofstream file(filename.str());
    if (!file.is_open()) return;

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    char timeBuffer[100];
    strftime(timeBuffer, sizeof(timeBuffer), "(%m/%d/%Y %I:%M:%S%p)", &tm);

    int processesInMem = 0;
    size_t fragmentation = 0;

    for (const auto& block : memoryBlocks) {
        if (!block.free) processesInMem++;
    }

    // Calculate external fragmentation (sum of all free blocks)
    for (const auto& block : memoryBlocks) {
        if (block.free) fragmentation += block.size;
    }

    file << "Timestamp: " << timeBuffer << "\n";
    file << "Number of processes in memory: " << processesInMem << "\n";
    file << "Total external fragmentation in KB: " << fragmentation << "\n\n";

    file << "----end---- = " << MEMORY_SIZE << "\n";

    // Print memory top-down
    size_t current = MEMORY_SIZE;
    for (auto it = memoryBlocks.rbegin(); it != memoryBlocks.rend(); ++it) {
        size_t end = current;
        size_t start = it->start;
        if (!it->free && it->process) {
            file << end << "\n";
            file << it->process->name << "\n";
            file << start << "\n";
        }
        current = start;
    }

    file << "----start---- = 0\n";
    file.close();
}

void MemoryManager::reset() {
    std::lock_guard<std::mutex> lock(memLock);
    memoryBlocks.clear();
    memoryBlocks.push_back({ 0, MEMORY_SIZE, true, nullptr });
}

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

size_t MemoryManager::getExternalFragmentation() const {
    size_t frag = 0;
    for (const auto& block : memoryBlocks) {
        if (block.free) frag += block.size;
    }
    return frag;
}

int MemoryManager::getProcessesInMemory() const {
    int count = 0;
    for (const auto& block : memoryBlocks) {
        if (!block.free) count++;
    }
    return count;
}

void MemoryManager::printMemoryStatus() const {
    std::cout << "\n=== Memory Layout ===\n";
    for (const auto& block : memoryBlocks) {
        std::cout << "[" << block.start << "-" << (block.start + block.size - 1)
            << "] " << (block.free ? "FREE" : "USED");
        if (!block.free && block.process) {
            std::cout << " (" << block.process->name << ")";
        }
        std::cout << "\n";
    }
    std::cout << "======================\n";
}

size_t MemoryManager::getUsedFrames() const {
    size_t used = 0;
    size_t memPerFrame = Config::getInstance().memPerFrame;
    for (const auto& block : memoryBlocks) {
        if (!block.free) {
            used += block.size;
        }
    }
    return used / memPerFrame;
}

size_t MemoryManager::getTotalFrames() const {
    return MEMORY_SIZE / Config::getInstance().memPerFrame;
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


bool MemoryManager::handlePageFault(std::shared_ptr<Process> process, size_t virtualPageNumber) {
    std::lock_guard<std::mutex> lock(memLock);

    // Find a free frame
    for (size_t i = 0; i < frames.size(); ++i) {
        if (!frames[i].occupied) {
            loadPage(process, virtualPageNumber, i);
            return true;
        }
    }

    // No free frame — need to evict a page (LRU policy)
    size_t lruIndex = 0;
    int oldestCycle = currentCycle.load();

    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].lastUsedCycle < oldestCycle) {
            lruIndex = i;
            oldestCycle = frames[i].lastUsedCycle;
        }
    }

    evictPage(frames[lruIndex].process, frames[lruIndex].virtualPageNumber);
    loadPage(process, virtualPageNumber, lruIndex);
    return true;
}

void MemoryManager::ensurePageLoaded(std::shared_ptr<Process> proc, size_t virtualAddress) {
    size_t virtualPage = virtualAddress / frameSize;
    if (!proc->loadedPages.count(virtualPage)) {
        std::string var;
        int val = 143;  //irdk what value this shd be help
        if (MemoryManager::getInstance().loadFromBackingStore(proc, virtualPage, var, val)) {
            proc->memory[var] = val; // Load from disk into memory
        }
        else {
            // First time this page is being used — initialize it
            proc->memory[var] = val;
            MemoryManager::getInstance().writeToBackingStore(proc, virtualPage, var, val);
        }
        proc->loadedPages.insert(virtualPage); // Mark page as loaded
    }
}

size_t MemoryManager::getFrameSize() const {
    return frameSize;
}

void MemoryManager::printMemoryStats(const std::vector<std::shared_ptr<Process>>& allProcesses) {
    std::cout << "==== Memory Stats ====\n";
    for (const auto& proc : allProcesses) {
        if (!proc) continue;

        std::cout << "PID " << proc->pid << " | Loaded Pages: ";
        for (auto page : proc->loadedPages) {
            std::cout << page << " ";
        }
        std::cout << "\n";
    }
    std::cout << "======================\n";
}


void MemoryManager::writeToBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber, const std::string& varName, int value) {
    std::ofstream outFile("csopesy-backing-store.txt", std::ios::app);
    outFile << "PID " << proc->pid << " PAGE " << pageNumber
        << " VAR " << varName << " VAL " << value << "\n";
    outFile.close();
}

bool MemoryManager::loadFromBackingStore(const std::shared_ptr<Process>& proc, size_t pageNumber, std::string& varName, int& value) {
    std::ifstream inFile("csopesy-backing-store.txt");
    std::string line;

    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string pidToken, pageToken, varToken, valToken;
        int pid, page;
        iss >> pidToken >> pid >> pageToken >> page >> varToken >> varName >> valToken >> value;

        if (pid == proc->pid && page == pageNumber) {
            inFile.close();
            return true;
        }
    }

    inFile.close();
    return false;
}
