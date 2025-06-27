#include "CLIManager.h"
#include "config.h"
#include <iostream>

int main() {
    Config& config = Config::getInstance();
    if (!config.loadFromFile("config.txt")) {
        std::cerr << "Failed to load config. Using defaults.\n";
    }


    CLIManager cli;
    cli.run();

    return 0;
}
