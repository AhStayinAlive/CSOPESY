#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    int numCPU;
    std::string scheduler;
    int quantumCycles;
    int batchProcessFreq;
    int minInstructions;
    int maxInstructions;
    int delayPerInstruction;
};

bool loadConfig(const std::string& filename, Config& config);

#endif