#include "SystemInfo.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/utsname.h>   // uname
#include <unistd.h>        // sysconf

// "model name\t: Intel(R) ..." -> "Intel(R) ..."
static std::string valueAfterColon(const std::string& line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos) return "";
    size_t start = line.find_first_not_of(" \t", pos + 1);
    return start == std::string::npos ? "" : line.substr(start);
}

bool SystemInfo::load() {
    std::ifstream cpu("/proc/cpuinfo");
    if (!cpu) return false;
    std::string line;
    while (std::getline(cpu, line)) {
        if (line.rfind("processor", 0) == 0)                       cpuCores++;
        else if (line.rfind("model name", 0) == 0 && cpuModel.empty()) cpuModel = valueAfterColon(line);
        else if (line.rfind("cache size", 0) == 0 && cacheSize.empty()) cacheSize = valueAfterColon(line);
        else if (line.rfind("address sizes", 0) == 0 && addressSizes.empty()) addressSizes = valueAfterColon(line);
    }

    std::ifstream mem("/proc/meminfo");
    while (std::getline(mem, line)) {
        std::istringstream in(line);
        std::string key;
        long value = 0;
        in >> key >> value;
        if (key == "MemTotal:")     memTotalKb = value;
        if (key == "MemAvailable:") memAvailableKb = value;
    }

    struct utsname u{};
    if (uname(&u) == 0) {                                          // system call
        kernel = std::string(u.sysname) + " " + u.release;
        machine = u.machine;
    }

    // Byte order: store 1 in a 32-bit integer and look at its first byte
    std::uint32_t one = 1;
    unsigned char first = 0;
    std::memcpy(&first, &one, 1);
    byteOrder = first == 1 ? "Little-endian" : "Big-endian";

    wordBits = static_cast<int>(sizeof(void*) * 8);
    pageSize = sysconf(_SC_PAGESIZE);
    return true;
}
