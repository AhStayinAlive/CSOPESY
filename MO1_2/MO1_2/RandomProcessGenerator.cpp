#include "RandomProcessGenerator.h"
#include "DeclareInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "PrintInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"
#include <chrono>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

// Static member initialization
std::mt19937 RandomProcessGenerator::generator;
bool RandomProcessGenerator::generatorInitialized = false;
std::uniform_int_distribution<int> RandomProcessGenerator::instructionCountDist;
std::uniform_int_distribution<int> RandomProcessGenerator::instructionTypeDist;
std::uniform_int_distribution<int> RandomProcessGenerator::valueDist;
std::uniform_int_distribution<int> RandomProcessGenerator::sleepTimeDist;
std::uniform_int_distribution<int> RandomProcessGenerator::loopCountDist;
std::uniform_int_distribution<int> RandomProcessGenerator::loopInstructionsDist;
std::uniform_int_distribution<int> RandomProcessGenerator::nestedLoopDist;

void RandomProcessGenerator::initializeGenerator() {
    if (!generatorInitialized) {
        // Use current time as seed if not explicitly set
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        generator.seed(static_cast<unsigned int>(seed));
        generatorInitialized = true;
    }
}

void RandomProcessGenerator::setRandomSeed(unsigned int seed) {
    generator.seed(seed);
    generatorInitialized = true;
}

std::string RandomProcessGenerator::generateVariableName(int index, const std::string& prefix) {
    return prefix + std::to_string(index);
}

std::string RandomProcessGenerator::generateResultName(int index, const std::string& prefix) {
    return prefix + std::to_string(index);
}

std::shared_ptr<Process> RandomProcessGenerator::generateRandomProcess(
    const std::string& name,
    int pid,
    const GenerationConfig& config) {

    initializeGenerator();

    auto process = std::make_shared<Process>();
    process->pid = pid;
    process->name = name;
    process->instructionPointer = 0;
    process->coreAssigned = -1;
    process->isRunning = false;
    process->isFinished = false;
    process->isDetached = false;
    process->completedInstructions = std::make_shared<std::atomic<int>>(0);

    try {
        // Setup distributions based on config
        std::uniform_int_distribution<int> instCountDist(config.minInstructions, config.maxInstructions);
        std::uniform_int_distribution<int> valDist(config.minValue, config.maxValue);
        std::uniform_int_distribution<int> sleepDist(config.minSleepTime, config.maxSleepTime);
        std::uniform_int_distribution<int> loopDist(config.minLoopCount, config.maxLoopCount);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        int numInstructions = instCountDist(generator);
        process->totalInstructions = numInstructions;

        // Always declare some initial variables (5-10)
        int initialVars = std::uniform_int_distribution<int>(5, 10)(generator);
        for (int i = 0; i < initialVars && process->instructions.size() < numInstructions; ++i) {
            std::string varName = generateVariableName(i);
            int value = valDist(generator);
            process->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));
        }

        // Generate remaining instructions based on probabilities
        while (process->instructions.size() < numInstructions) {
            double prob = probDist(generator);
            int currentIndex = static_cast<int>(process->instructions.size());

            if (prob < config.declareProbability) {
                // DECLARE instruction
                std::string varName = generateVariableName(currentIndex + 100);
                int value = valDist(generator);
                process->instructions.push_back(std::make_shared<DeclareInstruction>(varName, value));

            }
            else if (prob < config.declareProbability + config.arithmeticProbability) {
                // ADD or SUBTRACT instruction
                bool isAdd = std::uniform_int_distribution<int>(0, 1)(generator);
                std::string result = generateResultName(currentIndex, isAdd ? "sum" : "diff");
                std::string lhs = generateVariableName(currentIndex % initialVars);
                std::string rhs = generateVariableName((currentIndex + 1) % initialVars);

                if (isAdd) {
                    process->instructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs));
                }
                else {
                    process->instructions.push_back(std::make_shared<SubtractInstruction>(result, lhs, rhs));
                }

            }
            else if (prob < config.declareProbability + config.arithmeticProbability + config.printProbability) {
                // PRINT instruction
                std::vector<std::string> messages = {
                    "Hello world from " + name + "!",
                    "Process " + name + " executing instruction " + std::to_string(currentIndex),
                    "Greetings from process " + name,
                    "Debug output from " + name + " [" + std::to_string(currentIndex) + "]",
                    "Status update from " + name
                };
                std::string message = messages[std::uniform_int_distribution<int>(0, messages.size() - 1)(generator)];
                process->instructions.push_back(std::make_shared<PrintInstruction>(message));

            }
            else if (prob < config.declareProbability + config.arithmeticProbability +
                config.printProbability + config.sleepProbability) {
                // SLEEP instruction
                int sleepTime = sleepDist(generator);
                process->instructions.push_back(std::make_shared<SleepInstruction>(sleepTime));

            }
            else {
                // FOR loop instruction
                int loopCount = loopDist(generator);
                auto subInstructions = createRandomLoopInstructions(currentIndex, 0);
                process->instructions.push_back(std::make_shared<ForInstruction>(loopCount, subInstructions));
            }
        }

        // Ensure we don't exceed the target instruction count
        if (process->instructions.size() > numInstructions) {
            process->instructions.resize(numInstructions);
        }

        process->totalInstructions = static_cast<int>(process->instructions.size());

    }
    catch (const std::exception& e) {
        process->logs.push_back("Process generation failed: " + std::string(e.what()));
        std::cerr << "Error generating random process " << name << ": " << e.what() << std::endl;
    }

    return process;
}

std::shared_ptr<Process> RandomProcessGenerator::generateRandomProcess(
    const std::string& name,
    int pid,
    int minInstructions,
    int maxInstructions) {

    GenerationConfig config = createConfigFromParameters(minInstructions, maxInstructions);
    return generateRandomProcess(name, pid, config);
}

std::vector<std::shared_ptr<Instruction>> RandomProcessGenerator::createRandomLoopInstructions(
    int loopIndex,
    int nestingLevel) {

    std::vector<std::shared_ptr<Instruction>> subInstructions;
    std::uniform_int_distribution<int> loopInstDist(2, 5);
    std::uniform_real_distribution<double> probDist(0.0, 1.0);

    int numSubInstructions = loopInstDist(generator);

    for (int i = 0; i < numSubInstructions; ++i) {
        double prob = probDist(generator);

        if (prob < 0.6) {
            // PRINT instruction (most common in loops)
            std::string message = "Loop iteration " + std::to_string(i + 1) +
                " (nest level " + std::to_string(nestingLevel) + ")";
            subInstructions.push_back(std::make_shared<PrintInstruction>(message));

        }
        else if (prob < 0.8 && nestingLevel < 2) {
            // Nested FOR loop (only up to 2 levels deep to avoid excessive nesting)
            int nestedLoopCount = std::uniform_int_distribution<int>(2, 3)(generator);
            auto nestedSubInsts = createRandomLoopInstructions(loopIndex + i, nestingLevel + 1);
            subInstructions.push_back(std::make_shared<ForInstruction>(nestedLoopCount, nestedSubInsts));

        }
        else if (prob < 0.9) {
            // DECLARE instruction
            std::string varName = "loop_var" + std::to_string(loopIndex) + "_" + std::to_string(i);
            int value = std::uniform_int_distribution<int>(1, 50)(generator);
            subInstructions.push_back(std::make_shared<DeclareInstruction>(varName, value));

        }
        else {
            // ADD instruction
            std::string result = "loop_sum" + std::to_string(loopIndex) + "_" + std::to_string(i);
            std::string lhs = "var" + std::to_string(i % 5);
            std::string rhs = "var" + std::to_string((i + 1) % 5);
            subInstructions.push_back(std::make_shared<AddInstruction>(result, lhs, rhs));
        }
    }

    return subInstructions;
}

RandomProcessGenerator::GenerationConfig RandomProcessGenerator::createConfigFromParameters(
    int minIns, int maxIns, int minVal, int maxVal, int minSleep, int maxSleep) {

    GenerationConfig config;
    config.minInstructions = minIns;
    config.maxInstructions = maxIns;
    config.minValue = minVal;
    config.maxValue = maxVal;
    config.minSleepTime = minSleep;
    config.maxSleepTime = maxSleep;
    return config;
}

void RandomProcessGenerator::printProcessStatistics(const std::shared_ptr<Process>& process) {
    if (!process) {
        std::cout << "Invalid process pointer" << std::endl;
        return;
    }

    std::unordered_map<std::string, int> instructionCounts;

    for (const auto& instruction : process->instructions) {
        // This is a simplified way to count instruction types
        // In a real implementation, you'd need RTTI or virtual methods to identify types
        instructionCounts["Total"]++;
    }

    std::cout << "Process Statistics for " << process->name << ":" << std::endl;
    std::cout << "  PID: " << process->pid << std::endl;
    std::cout << "  Total Instructions: " << process->instructions.size() << std::endl;
    std::cout << "  Instruction Pointer: " << process->instructionPointer << std::endl;
    std::cout << "  Core Assigned: " << process->coreAssigned << std::endl;
    std::cout << "  Is Running: " << (process->isRunning ? "Yes" : "No") << std::endl;
    std::cout << "  Is Finished: " << (process->isFinished ? "Yes" : "No") << std::endl;
    std::cout << "  Memory Variables: " << process->memory.size() << std::endl;
}

std::string RandomProcessGenerator::getInstructionTypeDistribution(const std::shared_ptr<Process>& process) {
    if (!process) {
        return "Invalid process pointer";
    }

    std::ostringstream oss;
    oss << "Instruction Distribution for " << process->name << ":\n";
    oss << "  Total Instructions: " << process->instructions.size() << "\n";
    oss << "  Memory Variables: " << process->memory.size() << "\n";
    oss << "  Logs Generated: " << process->logs.size();

    return oss.str();
}