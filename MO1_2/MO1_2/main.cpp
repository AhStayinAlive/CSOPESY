#include "CLIManager.h"
#include "config.h"
#include <iostream>

int main() {
    Config& config = Config::getInstance();

    CLIManager cli;
    cli.run();

    return 0;
}
