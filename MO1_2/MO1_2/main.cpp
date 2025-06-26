#include "CLIManager.h"
#include "config.h"
#include <iostream>

// Function declarations from demo_random_generator.cpp
void demonstrateBasicGeneration();
void demonstrateConfigurableGeneration();
void demonstrateSpecializedProcesses();
void demonstrateBatchGeneration();
void demonstrateWithConfig();
void runAllDemonstrations();

int main() {
    // Optional: Run demo at startup (comment out if not needed)
    // std::cout << "Running Random Process Generator Demo...\n";
    // runAllDemonstrations();
    // std::cout << "\nStarting main application...\n\n";

    CLIManager cli;
    cli.run();
    return 0;
}