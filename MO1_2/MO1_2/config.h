#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {
private:
    Config() = default; // Private constructor for singleton

public:
    // Singleton instance getter
    static Config& getInstance();

    // Delete copy constructor and assignment operator
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // Configuration parameters with default values
    int numCPU = 4;
    std::string scheduler = "FCFS";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minInstructions = 1000;
    int maxInstructions = 2000;
    int delayPerInstruction = 100;

    // Method to load configuration from file
    bool loadFromFile(const std::string& filename);
};

#endif // CONFIG_H