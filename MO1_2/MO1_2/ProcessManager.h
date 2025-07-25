﻿#pragma once
#include <memory>
#include <string>
#include <vector>
#include "process.h"

class ProcessManager {
public:
    static std::shared_ptr<Process> createProcess(const std::string& name, int pid, int minInstructions, int maxInstructions, size_t memPerProc);
    static std::shared_ptr<Process> createUniqueNamedProcess(int minIns, int maxIns, size_t memPerProc);
    static std::shared_ptr<Process> createNamedProcess(const std::string& name);
    static std::shared_ptr<Process> findByName(const std::string& name);
    static void addProcess(std::shared_ptr<Process> proc);
    static std::vector<std::shared_ptr<Process>> getAllProcesses();
    static std::vector<std::shared_ptr<Process>> getWaitingProcesses();
    static std::vector<std::shared_ptr<Process>> getRunningProcesses();     
    static std::vector<std::shared_ptr<Process>> getFinishedProcesses();    
    static std::shared_ptr<Process> findByPid(int pid);
    static int getProcessCount();
    static int getRunningProcessCount();
    static int getFinishedProcessCount();
    static double getCpuUtilization();
    static void clearAllProcesses();
};
