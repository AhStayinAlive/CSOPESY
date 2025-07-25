#include "CLIManager.h"
#include "config.h"
#include "MemoryManager.h"
#include <iostream>

int main() {
    Config& config = Config::getInstance();

    // Load configuration (assuming config.loadFromFile is implemented)
    if (!config.loadFromFile("config.txt")) {
        std::cout << "Warning: Could not load config.txt, using default values.\n";
    }

    // Display configuration
    std::cout << "=== CSOPESY Configuration ===\n";
    std::cout << "Number of CPUs: " << config.numCPU << "\n";
    std::cout << "Scheduler: " << config.scheduler << "\n";
    std::cout << "Quantum Cycles: " << config.quantumCycles << "\n";
    std::cout << "Batch Process Frequency: " << config.batchProcessFreq << "\n";
    std::cout << "Min Instructions: " << config.minInstructions << "\n";
    std::cout << "Max Instructions: " << config.maxInstructions << "\n";
    std::cout << "Delay per Instruction: " << config.delayPerInstruction << "ms\n";
    std::cout << "Total Memory: " << config.maxOverallMem << " bytes\n";
    std::cout << "Memory per Process: " << config.memPerProc << " bytes\n";
    std::cout << "Memory per Frame: " << config.memPerFrame << " bytes\n";
    std::cout << "==============================\n\n";

    CLIManager cli;
    cli.run();

    return 0;
}