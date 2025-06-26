#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    int numCPU = 0;
    std::string scheduler = ""; // or empty string if preferred
    int quantumCycles = 0;
    int batchProcessFreq = 1;
    int minInstructions = 0;
    int maxInstructions = 0;
    int delayPerInstruction = 1;
};

bool loadConfig(const std::string& filename, Config& config);

#endif
