#pragma once

#include "process.h"
#include <memory>
#include <string>
#include <vector>
#include <random>

class RandomProcessGenerator {
private:
    static std::mt19937 generator;
    static bool generatorInitialized;

    // Random distribution generators
    static std::uniform_int_distribution<int> instructionCountDist;
    static std::uniform_int_distribution<int> instructionTypeDist;
    static std::uniform_int_distribution<int> valueDist;
    static std::uniform_int_distribution<int> sleepTimeDist;
    static std::uniform_int_distribution<int> loopCountDist;
    static std::uniform_int_distribution<int> loopInstructionsDist;
    static std::uniform_int_distribution<int> nestedLoopDist;

    // Helper methods
    static void initializeGenerator();
    static std::string generateVariableName(int index, const std::string& prefix = "var");
    static std::string generateResultName(int index, const std::string& prefix = "result");
    static std::shared_ptr<Instruction> createRandomInstruction(int instructionIndex, int totalInstructions);
    static std::vector<std::shared_ptr<Instruction>> createRandomLoopInstructions(int loopIndex, int nestingLevel = 0);
    static std::shared_ptr<Instruction> createRandomBasicInstruction(int instructionIndex);

public:
    // Configuration structure for process generation
    struct GenerationConfig {
        int minInstructions = 10;
        int maxInstructions = 50;
        int minValue = 1;
        int maxValue = 100;
        int minSleepTime = 100;
        int maxSleepTime = 1000;
        int minLoopCount = 2;
        int maxLoopCount = 5;
        int maxNestingLevel = 3;
        double sleepProbability = 0.2;       // 20% chance for sleep instructions
        double loopProbability = 0.15;       // 15% chance for loop instructions
        double declareProbability = 0.25;    // 25% chance for declare instructions
        double arithmeticProbability = 0.3;   // 30% chance for add/subtract
        double printProbability = 0.1;       // 10% chance for print instructions
    };

    // Main generation methods
    static std::shared_ptr<Process> generateRandomProcess(
        const std::string& name,
        int pid,
        const GenerationConfig& config = GenerationConfig()
    );

    static std::shared_ptr<Process> generateRandomProcess(
        const std::string& name,
        int pid,
        int minInstructions,
        int maxInstructions
    );

    // Utility methods
    static void setRandomSeed(unsigned int seed);
    static GenerationConfig createConfigFromParameters(
        int minIns, int maxIns,
        int minVal = 1, int maxVal = 100,
        int minSleep = 100, int maxSleep = 1000
    );

    // Statistics and debugging
    static void printProcessStatistics(const std::shared_ptr<Process>& process);
    static std::string getInstructionTypeDistribution(const std::shared_ptr<Process>& process);
};