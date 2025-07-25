#pragma once
#include <memory>
#include "process.h"

class ConsoleView {
public:
    static void show(const std::shared_ptr<Process>& proc);               // <- NEW
    static void displayProcessScreen(const std::shared_ptr<Process>& proc);
    static void clearScreen();
};
