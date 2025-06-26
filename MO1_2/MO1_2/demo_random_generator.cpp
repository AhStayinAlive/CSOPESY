#include "RandomProcessGenerator.h"
#include "process.h"
#include "config.h"
#include <iostream>
#include <vector>
#include <memory>

// Function declarations from enhanced process.cpp
std::shared_ptr<Process> generateComputeIntensiveProcess(const std::string& name, int pid, int minIns, int maxIns);
std::shared_ptr<Process> generateIOIntensiveProcess(const std::string& name, int pid, int minIns, int maxIns);
std::shared_ptr<Process> generateBalancedProcess(const std::string& name, int pid, int minIns, int maxIns);
std::vector<std::shared_ptr<Process>> generateProcessBatch(const std::string& baseNamePrefix, int startingPid, int count, int minIns, int maxIns);
void printProcessInstructionSummary(const std::shared_ptr<Process>& proc);
bool validateGeneratedProcess(const std::shared_ptr<Process>& proc);

void demonstrateBasicGeneration() {
    std::cout << "\n=== Basic Random Process Generation Demo ===" << std::endl;

    // Generate a simple random process
    auto proc1 = RandomProcessGenerator::generateRandomProcess("TestProcess1", 1001, 10, 20);
    if (validateGeneratedProcess(proc1)) {
        RandomProcessGenerator::printProcessStatistics(proc1);
        std::cout << RandomProcessGenerator::getInstructionTypeDistribution(proc1) << std::endl;
    }

    std::cout << "\n";
}

void demonstrateConfigurableGeneration() {
    std::cout << "\n=== Configurable Random Process Generation Demo ===" << std::endl;

    // Create custom configuration
    RandomProcessGenerator::GenerationConfig config;
    config.minInstructions = 15;
    config.maxInstructions = 25;
    config.sleepProbability = 0.3;   // Higher sleep probability
    config.loopProbability = 0.2;    // Higher loop probability
    config.printProbability = 0.25;  // Higher print probability
    config.declareProbability = 0.15;
    config.arithmeticProbability = 0.1;

    auto proc2 = RandomProcessGenerator::generateRandomProcess("ConfigurableProcess", 1002, config);
    if (validateGeneratedProcess(proc2)) {
        RandomProcessGenerator::printProcessStatistics(proc2);
        printProcessInstructionSummary(proc2);
    }

    std::cout << "\n";
}

void demonstrateSpecializedProcesses() {
    std::cout << "\n=== Specialized Process Types Demo ===" << std::endl;

    // Generate different types of processes
    auto computeProc = generateComputeIntensiveProcess("ComputeHeavy", 2001, 15, 30);
    auto ioProc = generateIOIntensiveProcess("IOHeavy", 2002, 15, 30);
    auto balancedProc = generateBalancedProcess("Balanced", 2003, 15, 30);

    std::cout << "\nCompute-Intensive Process:" << std::endl;
    if (validateGeneratedProcess(computeProc)) {
        RandomProcessGenerator::printProcessStatistics(computeProc);
    }

    std::cout << "\nI/O-Intensive Process:" << std::endl;
    if (validateGeneratedProcess(ioProc)) {
        RandomProcessGenerator::printProcessStatistics(ioProc);
    }

    std::cout << "\nBalanced Process:" << std::endl;
    if (validateGeneratedProcess(balancedProc)) {
        RandomProcessGenerator::printProcessStatistics(balancedProc);
    }
}

void demonstrateBatchGeneration() {
    std::cout << "\n=== Batch Process Generation Demo ===" << std::endl;

    // Generate a batch of processes
    auto processBatch = generateProcessBatch("BatchProc", 3000, 5, 10, 20);

    std::cout << "Generated " << processBatch.size() << " processes in batch:" << std::endl;
    for (const auto& proc : processBatch) {
        if (validateGeneratedProcess(proc)) {
            std::cout << "  - " << proc->name << " (PID: " << proc->pid
                << ", Instructions: " << proc->instructions.size() << ")" << std::endl;
        }
    }
}

void demonstrateWithConfig() {
    std::cout << "\n=== Integration with Config System Demo ===" << std::endl;

    // Load configuration from file
    Config& config = Config::getInstance();
    if (config.loadFromFile("config.txt")) {
        std::cout << "Loaded configuration from config.txt" << std::endl;

        // Generate process using config parameters
        auto configProc = RandomProcessGenerator::generateRandomProcess(
            "ConfigBasedProcess",
            4001,
            config.minInstructions,
            config.maxInstructions
        );

        if (validateGeneratedProcess(configProc)) {
            std::cout << "Generated process based on config.txt:" << std::endl;
            RandomProcessGenerator::printProcessStatistics(configProc);
        }
    }
    else {
        std::cout << "Could not load config.txt, using default values" << std::endl;

        // Use default config values
        RandomProcessGenerator::GenerationConfig defaultConfig;
        auto defaultProc = RandomProcessGenerator::generateRandomProcess(
            "DefaultProcess",
            4002,
            defaultConfig
        );

        if (validateGeneratedProcess(defaultProc