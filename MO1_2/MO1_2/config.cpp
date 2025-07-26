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
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (!(iss >> key >> value)) continue;

        if (key == "num-cpu") numCPU = std::stoi(value);
        else if (key == "scheduler") scheduler = value;
        else if (key == "quantum-cycles") quantumCycles = std::stoi(value);
        else if (key == "batch-process-freq") batchProcessFreq = std::stoi(value);
        else if (key == "min-ins") minInstructions = std::stoi(value);
        else if (key == "max-ins") maxInstructions = std::stoi(value);
        else if (key == "delay-per-exec" || key == "delays-per-exec") delayPerInstruction = std::stoi(value);
		else if (key == "max-overall-mem") maxOverallMem = std::stoul(value);
		else if (key == "mem-per-frame") memPerFrame = std::stoul(value);
        else if (key == "min-mem-per-proc") minMemPerProc = std::stoul(value);
        else if (key == "max-mem-per-proc") maxMemPerProc = std::stoul(value);
		
    }

    return true;
}
