#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "config.h"

void startScheduler(const Config& config);
void stopScheduler();
void addProcess(std::shared_ptr<Process> p);
void generateReport();
extern bool running;

#endif
