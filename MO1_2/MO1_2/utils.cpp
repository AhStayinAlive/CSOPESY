#include "utils.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);  // Safe Windows version
    std::stringstream ss;
    ss << std::put_time(&ltm, "(%m/%d/%Y %I:%M:%S%p)");
    return ss.str();
}

bool fileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void logToFile(const std::string& processName, const std::string& message, int coreId) {
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
