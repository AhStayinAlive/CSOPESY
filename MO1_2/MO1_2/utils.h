#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

inline std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);
    std::stringstream ss;
    ss << std::put_time(&ltm, "(%m/%d/%Y %I:%M:%S%p)");
    return ss.str();
}

inline bool fileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

inline void logToFile(const std::string& processName, const std::string& message, int coreId = -1) {
    std::string filename = processName + ".txt";
    std::ofstream out(filename, std::ios::app);

    if (!fileExists(filename)) {
        out << "Process name: " << processName << std::endl;
        out << "Logs:" << std::endl << std::endl;
    }

    out << getCurrentTimestamp();
    if (coreId >= 0) out << " Core:" << coreId;
    out << " \"" << message << "\"" << std::endl;
}

#endif