#pragma once
#include <memory>
#include <string>
#include <vector>
#include "process.h"

class ProcessManager {
public:
    static std::shared_ptr<Process> createProcess(const std::string& name, int pid, int minInstructions, int maxInstructions);
    static std::shared_ptr<Process> createUniqueNamedProcess(int minIns, int maxIns);
    static std::shared_ptr<Process> createNamedProcess(const std::string& name);
    static std::shared_ptr<Process> findByName(const std::string& name);
    static void addProcess(std::shared_ptr<Process> proc);
    static std::vector<std::shared_ptr<Process>> getAllProcesses();  // ← Needed for showProcessList
};
