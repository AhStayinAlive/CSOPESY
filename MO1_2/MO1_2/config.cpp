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

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                try {
                    if (key == "numCPU") {
                        numCPU = std::stoi(value);
                        if (numCPU <= 0) {
                            std::cerr << "Warning: numCPU must be greater than 0. Using default value: 4" << std::endl;
                            numCPU = 4;
                        }
                    }
                    else if (key == "scheduler") {
                        scheduler = value;
                    }
                    else if (key == "quantumCycles") {
                        quantumCycles = std::stoi(value);
                        if (quantumCycles <= 0) {
                            std::cerr << "Warning: quantumCycles must be greater than 0. Using default value: 5" << std::endl;
                            quantumCycles = 5;
                        }
                    }
                    else if (key == "batchProcessFreq") {
                        batchProcessFreq = std::stoi(value);
                        if (batchProcessFreq <= 0) {
                            std::cerr << "Warning: batchProcessFreq must be greater than 0. Using default value: 1" << std::endl;
                            batchProcessFreq = 1;
                        }
                    }
                    else if (key == "minInstructions") {
                        minInstructions = std::stoi(value);
                        if (minInstructions <= 0) {
                            std::cerr << "Warning: minInstructions must be greater than 0. Using default value: 1000" << std::endl;
                            minInstructions = 1000;
                        }
                    }
                    else if (key == "maxInstructions") {
                        maxInstructions = std::stoi(value);
                        if (maxInstructions <= 0) {
                            std::cerr << "Warning: maxInstructions must be greater than 0. Using default value: 2000" << std::endl;
                            maxInstructions = 2000;
                        }
                    }
                    else if (key == "delayPerInstruction") {
                        delayPerInstruction = std::stoi(value);
                        if (delayPerInstruction < 0) {
                            std::cerr << "Warning: delayPerInstruction must be non-negative. Using default value: 100" << std::endl;
                            delayPerInstruction = 100;
                        }
                    }
                    else {
                        std::cerr << "Warning: Unknown config key '" << key << "' at line " << lineNumber << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing config value for '" << key << "' at line " << lineNumber
                        << ": " << e.what() << std::endl;
                }
            }
        }
    }

    // Validate that maxInstructions >= minInstructions
    if (maxInstructions < minInstructions) {
        std::cerr << "Warning: maxInstructions (" << maxInstructions
            << ") is less than minInstructions (" << minInstructions
            << "). Swapping values." << std::endl;
        std::swap(minInstructions, maxInstructions);
    }

    file.close();

    // Print loaded configuration
    std::cout << "Configuration loaded successfully:" << std::endl;
    std::cout << "  numCPU: " << numCPU << std::endl;
    std::cout << "  scheduler: " << scheduler << std::endl;
    std::cout << "  quantumCycles: " << quantumCycles << std::endl;
    std::cout << "  batchProcessFreq: " << batchProcessFreq << std::endl;
    std::cout << "  minInstructions: " << minInstructions << std::endl;
    std::cout << "  maxInstructions: " << maxInstructions << std::endl;
    std::cout << "  delayPerInstruction: " << delayPerInstruction << std::endl;

    return true;
}