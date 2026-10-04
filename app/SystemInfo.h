/*
 * SystemInfo.h - computer architecture information of the machine.
 * Reads /proc/cpuinfo and /proc/meminfo (kernel-provided virtual files)
 * and uses the uname() and sysconf() system calls.
 */
#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <string>

struct SystemInfo {
    std::string cpuModel;
    int cpuCores = 0;
    std::string cacheSize;
    std::string addressSizes;     // physical / virtual address bits
    long memTotalKb = 0;
    long memAvailableKb = 0;
    std::string kernel;           // e.g. "Linux 7.0.0-30-generic"
    std::string machine;          // e.g. "x86_64"
    std::string byteOrder;        // "Little-endian" / "Big-endian"
    int wordBits = 0;             // 64 on a 64-bit system
    long pageSize = 0;            // memory page size in bytes

    bool load();
};

#endif
