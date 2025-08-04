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
    std::string scheduler = "ROUND_ROBIN";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minInstructions = 1000;
    int maxInstructions = 2000;
    int delayPerInstruction = 100;

    int maxOverallMem = 512;     // total bytes of physical memory
    int memPerFrame = 256;       // size per frame
    int minMemPerProc = 512;     // min memory per process
    int maxMemPerProc = 512;     // max memory per process

    // Method to load configuration from file
    bool loadFromFile(const std::string& filename);
};

#endif // CONFIG_H
