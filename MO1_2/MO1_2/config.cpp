#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string key;
    while (file >> key) {
        if (key == "num-cpu") file >> numCPU;
        else if (key == "scheduler") file >> scheduler;
        else if (key == "quantum-cycles") file >> quantumCycles;
        else if (key == "batch-process-freq") file >> batchProcessFreq;
        else if (key == "min-ins") file >> minInstructions;
        else if (key == "max-ins") file >> maxInstructions;
        else if (key == "delays-per-exec") file >> delayPerInstruction;
    }

    std::cout << "Configuration loaded successfully:\n";
    std::cout << "  numCPU: " << numCPU << "\n";
    std::cout << "  scheduler: " << scheduler << "\n";
    std::cout << "  quantumCycles: " << quantumCycles << "\n";
    std::cout << "  batchProcessFreq: " << batchProcessFreq << "\n";
    std::cout << "  minInstructions: " << minInstructions << "\n";
    std::cout << "  maxInstructions: " << maxInstructions << "\n";
    std::cout << "  delayPerInstruction: " << delayPerInstruction << "\n";

    return true;
}