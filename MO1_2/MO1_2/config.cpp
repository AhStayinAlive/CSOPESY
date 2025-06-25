#include "config.h"
#include <fstream>
#include <sstream>

bool loadConfig(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file) return false;

    std::string key;
    while (file >> key) {
        if (key == "num-cpu") file >> config.numCPU;
        else if (key == "scheduler") file >> config.scheduler;
        else if (key == "quantum-cycles") file >> config.quantumCycles;
        else if (key == "batch-process-freq") file >> config.batchProcessFreq;
        else if (key == "min-ins") file >> config.minInstructions;
        else if (key == "max-ins") file >> config.maxInstructions;
        else if (key == "delays-per-exec") file >> config.delayPerInstruction;
    }

    return true;
}