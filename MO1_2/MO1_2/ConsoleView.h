#pragma once
#include <memory>
#include "process.h"
class ConsoleView {
public:
    static void show(const std::shared_ptr<Process>& proc);
    static void displayProcessScreen(const std::shared_ptr<Process>& proc);
    static void clearScreen();
private:
    static void displayColoredLogs(const std::shared_ptr<Process>& proc, bool showAll = false);  // Make this static
};