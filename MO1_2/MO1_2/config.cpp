// config.cpp
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
        else if (key == "delay-per-exec") file >> delayPerInstruction;
        else if (key == "max-overall-mem") file >> maxOverallMem;
        else if (key == "mem-per-frame") file >> memPerFrame;
        else if (key == "min-mem-per-proc") file >> minMemPerProc;
        else if (key == "max-mem-per-proc") file >> maxMemPerProc;
    }
    return true;
}