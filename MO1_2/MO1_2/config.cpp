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
        std::cerr << "Error: Could not open config file: " << filename << std::endl;
        std::cerr << "Using default values..." << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        // Trim whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        // Skip empty lines or comments
        if (line.empty() || line[0] == '#') continue;

        auto delimiterPos = line.find(':');
        if (delimiterPos == std::string::npos) continue;

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        try {
            if (key == "numCPU") {
                numCPU = std::stoi(value);
                if (numCPU <= 0) numCPU = 4;
            }
            else if (key == "scheduler") {
                scheduler = value;
            }
            else if (key == "quantumCycles") {
                quantumCycles = std::stoi(value);
                if (quantumCycles <= 0) quantumCycles = 5;
            }
            else if (key == "batchProcessFreq") {
                batchProcessFreq = std::stoi(value);
                if (batchProcessFreq <= 0) batchProcessFreq = 1;
            }
            else if (key == "minInstructions") {
                minInstructions = std::stoi(value);
                if (minInstructions <= 0) minInstructions = 1000;
            }
            else if (key == "maxInstructions") {
                maxInstructions = std::stoi(value);
                if (maxInstructions <= 0) maxInstructions = 2000;
            }
            else if (key == "delayPerInstruction") {
                delayPerInstruction = std::stoi(value);
                if (delayPerInstruction < 0) delayPerInstruction = 100;
            }
            else {
                std::cerr << "Unknown config key at line " << lineNumber << ": " << key << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing value for '" << key << "' at line " << lineNumber << ": " << e.what() << std::endl;
        }
    }

    if (maxInstructions < minInstructions) {
        std::swap(minInstructions, maxInstructions);
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