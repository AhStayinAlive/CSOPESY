#ifndef UTILS_H
#define UTILS_H

#include <string>

// Declaration only
std::string getCurrentTimestamp();
bool fileExists(const std::string& filename);
void logToFile(const std::string& processName, const std::string& message, int coreId = -1);

#endif
