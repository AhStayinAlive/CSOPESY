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
    void printSystemProcessSMI();
private:
    void handleCommand(const std::string& input);
    std::vector<std::string> tokenize(const std::string& input) const;
    void showProcessList() const;
    void printVMStat();

    bool generating;
    std::thread schedulerThread;
};