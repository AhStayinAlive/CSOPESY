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

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                if (key == "numCPU") numCPU = std::stoi(value);
                else if (key == "scheduler") scheduler = value;
                else if (key == "quantumCycles") quantumCycles = std::stoi(value);
                else if (key == "batchProcessFreq") batchProcessFreq = std::stoi(value);
                else if (key == "minInstructions") minInstructions = std::stoi(value);
                else if (key == "maxInstructions") maxInstructions = std::stoi(value);
                else if (key == "delayPerInstruction") delayPerInstruction = std::stoi(value);
            }
        }
    }

    file.close();
    return true;
}
