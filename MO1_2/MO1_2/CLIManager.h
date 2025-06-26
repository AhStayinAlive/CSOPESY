#pragma once
#include <string>
#include <vector>
#include <thread>

class CLIManager {
public:
    CLIManager();
    void run();
    void stopScheduler();
    void showHelp() const;

private:
    void handleCommand(const std::string& input);
    std::vector<std::string> tokenize(const std::string& input) const;
    void showProcessList() const;

    bool generating;
    std::thread schedulerThread;
};