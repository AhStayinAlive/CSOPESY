#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Find the equals sign
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        // Extract key and value
        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Remove quotes if present
        if (value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }

        // Parse configuration values
        try {
            if (key == "num-cpu") {
                numCPU = std::stoi(value);
            }
            else if (key == "scheduler") {
                scheduler = value;
            }
            else if (key == "quantum-cycles") {
                quantumCycles = std::stoi(value);
            }
            else if (key == "batch-process-freq") {
                batchProcessFreq = std::stoi(value);
            }
            else if (key == "min-ins") {
                minInstructions = std::stoi(value);
            }
            else if (key == "max-ins") {
                maxInstructions = std::stoi(value);
            }
            else if (key == "delay-per-exec") {
                delayPerInstruction = std::stoi(value);
            }
            else if (key == "max-overall-mem") {
                maxOverallMem = std::stoul(value);
            }
            else if (key == "mem-per-proc") {
                memPerProc = std::stoul(value);
            }
            else if (key == "mem-per-frame") {
                memPerFrame = std::stoul(value);
            }
            else {
                std::cerr << "Warning: Unknown configuration key: " << key << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error parsing config value for key '" << key << "': " << e.what() << std::endl;
        }
    }

    file.close();
    return true;
}