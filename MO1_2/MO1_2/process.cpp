#include "process.h"
#include "RandomProcessGenerator.h"
#include "DeclareInstruction.h"
#include "PrintInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include <random>
#include <memory>
#include <sstream>
#include <iostream>

// Legacy function - maintained for backward compatibility
std::shared_ptr<Process> generateRandomProcess(std::string name, int pid, int minIns, int maxIns) {
    return RandomProcessGenerator::generateRandomProcess(name, pid, minIns, maxIns);
}

// Enhanced random process generation with custom configuration
std::shared_ptr<Process> generateAdvancedRandomProcess(
    const std::string& name,
    int pid,
    const RandomProcessGenerator::GenerationConfig& config) {

    return RandomProcessGenerator::generateRandomProcess(name, pid, config);
}

// Generate a process with specific instruction type probabilities
std::shared_ptr<Process> generateWeightedRandomProcess(
    const std::string& name,
    int pid,
    int minIns,
    int maxIns,
    double sleepWeight = 0.2,
    double loopWeight = 0.15,
    double printWeight = 0.2) {

    RandomProcessGenerator::GenerationConfig config;
    config.minInstructions = minIns;
    config.maxInstructions = maxIns;
    config.sleepProbability = sleepWeight;
    config.loopProbability = loopWeight;
    config.printProbability = printWeight;

    // Adjust other probabilities to maintain total = 1.0
    double remaining = 1.0 - sleepWeight - loopWeight - printWeight;
    config.declareProbability = remaining * 0.4;      // 40% of remaining
    config.arithmeticProbability = remaining * 0.6;   // 60% of remaining

    return RandomProcessGenerator::generateRandomProcess(name, pid, config);
}

// Generate a compute-intensive process (more arithmetic, fewer sleeps)
std::shared_ptr<Process> generateComputeIntensiveProcess(
    const std::string& name,
    int pid,
    int minIns,
    int maxIns) {

    RandomProcessGenerator::GenerationConfig config;
    config.minInstructions = minIns;
    config.maxInstructions = maxIns;
    config.sleepProbability = 0.05;      // Very few sleeps
    config.loopProbability = 0.25;       // More loops
    config.printProbability = 0.1;       // Minimal printing
    config.declareProbability = 0.2;     // Some declarations
    config.arithmeticProbability = 0.4;  // Heavy arithmetic

    return RandomProcessGenerator::generateRandomProcess(name, pid, config);
}

// Generate an I/O intensive process (more sleeps and prints)
std::shared_ptr<Process> generateIOIntensiveProcess(
    const std::string& name,
    int pid,
    int minIns,
    int maxIns) {

    RandomProcessGenerator::GenerationConfig config;
    config.minInstructions = minIns;
    config.maxInstructions = maxIns;
    config.sleepProbability = 0.4;       // Frequent sleeps (simulating I/O waits)
    config.loopProbability = 0.1;        // Fewer loops
    config.printProbability = 0.3;       // Frequent printing
    config.declareProbability = 0.1;     // Minimal declarations
    config.arithmeticProbability = 0.1;  // Minimal arithmetic

    return RandomProcessGenerator::generateRandomProcess(name, pid, config);
}

// Generate a balanced process with good mix of all instruction types
std::shared_ptr<Process> generateBalancedProcess(
    const std::string& name,
    int pid,
    int minIns,
    int maxIns) {

    RandomProcessGenerator::GenerationConfig config;
    config.minInstructions = minIns;
    config.maxInstructions = maxIns;
    config.sleepProbability = 0.15;
    config.loopProbability = 0.15;
    config.printProbability = 0.2;
    config.declareProbability = 0.25;
    config.arithmeticProbability = 0.25;

    return RandomProcessGenerator::generateRandomProcess(name, pid, config);
}

// Generate multiple processes with different characteristics
std::vector<std::shared_ptr<Process>> generateProcessBatch(
    const std::string& baseNamePrefix,
    int startingPid,
    int count,
    int minIns,
    int maxIns) {

    std::vector<std::shared_ptr<Process>> processes;
    processes.reserve(count);

    for (int i = 0; i < count; ++i) {
        std::string name = baseNamePrefix + std::to_string(i + 1);
        int pid = startingPid + i;

        // Vary process types for diversity
        std::shared_ptr<Process> proc;
        switch (i % 4) {
        case 0:
            proc = generateComputeIntensiveProcess(name, pid, minIns, maxIns);
            break;
        case 1:
            proc = generateIOIntensiveProcess(name, pid, minIns, maxIns);
            break;
        case 2:
            proc = generateBalancedProcess(name, pid, minIns, maxIns);
            break;
        default:
            proc = generateRandomProcess(name, pid, minIns, maxIns);
            break;
        }

        processes.push_back(proc);
    }

    return processes;
}

// Utility function to validate process generation
bool validateGeneratedProcess(const std::shared_ptr<Process>& proc) {
    if (!proc) {
        std::cerr << "Error: Null process pointer" << std::endl;
        return false;
    }

    if (proc->name.empty()) {
        std::cerr << "Error: Process has empty name" << std::endl;
        return false;
    }

    if (proc->instructions.empty()) {
        std::cerr << "Error: Process " << proc->name << " has no instructions" << std::endl;
        return false;
    }

    if (proc->totalInstructions != static_cast<int>(proc->instructions.size())) {
        std::cerr << "Warning: Process " << proc->name
            << " instruction count mismatch: total=" << proc->totalInstructions
            << ", actual=" << proc->instructions.size() << std::endl;
        proc->totalInstructions = static_cast<int>(proc->instructions.size());
    }

    return true;
}

// Debug function to print process instruction summary
void printProcessInstructionSummary(const std::shared_ptr<Process>& proc) {
    if (!proc) {
        std::cout << "Invalid process pointer" << std::endl;
        return;
    }

    std::cout << "\n=== Process " << proc->name << " Summary ===" << std::endl;
    std::cout << "PID: " << proc->pid << std::endl;
    std::cout << "Total Instructions: " << proc->instructions.size() << std::endl;
    std::cout << "Memory Variables: " << proc->memory.size() << std::endl;

    // Print first few instructions for debugging
    std::cout << "First 5 instructions:" << std::endl;
    for (size_t i = 0; i < std::min(proc->instructions.size(), size_t(5)); ++i) {
        std::cout << "  [" << i << "] Instruction (type varies)" << std::endl;
    }

    if (proc->instructions.size() > 5) {
        std::cout << "  ... and " << (proc->instructions.size() - 5) << " more instructions" << std::endl;
    }
    std::cout << "================================" << std::endl;
}