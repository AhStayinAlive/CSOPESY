#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {
private:
    Config() = default;

public:
    static Config& getInstance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    int numCPU = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minInstructions = 1000;
    int maxInstructions = 2000;
    int delayPerInstruction = 100;

    // Memory management parameters
    size_t maxOverallMem = 16384;
    size_t memPerFrame = 16;
    size_t minMemPerProc = 4096;
    size_t maxMemPerProc = 4096;

    // Method to load configuration from file
    bool loadFromFile(const std::string& filename);
};

#endif // CONFIG_H
