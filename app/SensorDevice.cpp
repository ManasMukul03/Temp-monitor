#include "SensorDevice.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>       // open
#include <unistd.h>      // pread, close
#include <sys/ioctl.h>   // ioctl

SensorDevice::SensorDevice(const std::string& path) : path_(path) {
    fd_ = ::open(path.c_str(), O_RDWR);                       // system call
    if (fd_ < 0)
        throw std::runtime_error("cannot open " + path + ": " + std::strerror(errno));
}

SensorDevice::~SensorDevice() {
    if (fd_ >= 0)
        ::close(fd_);                                          // system call
}

// The driver returns text like "36250\n" (milli-degrees Celsius).
// pread(..., 0) reads from offset 0 every time, so each call gets a fresh value.
double SensorDevice::readCelsius() {
    char buf[32] = {0};
    ssize_t n = ::pread(fd_, buf, sizeof(buf) - 1, 0);        // system call -> ts_read()
    if (n <= 0)
        throw std::runtime_error(std::string("read failed: ") + std::strerror(errno));
    return std::atoi(buf) / 1000.0;
}

void SensorDevice::doIoctl(unsigned long cmd, void* arg, const char* name) {
    if (::ioctl(fd_, cmd, arg) < 0)                            // system call -> ts_ioctl()
        throw std::runtime_error(std::string(name) + " failed: " + std::strerror(errno));
}

void SensorDevice::setInterval(unsigned ms) {
    __u32 value = ms;
    doIoctl(TS_IOC_SET_INTERVAL, &value, "SET_INTERVAL");
}

unsigned SensorDevice::getInterval() {
    __u32 value = 0;
    doIoctl(TS_IOC_GET_INTERVAL, &value, "GET_INTERVAL");
    return value;
}

unsigned SensorDevice::getStatus() {
    __u32 value = 0;
    doIoctl(TS_IOC_GET_STATUS, &value, "GET_STATUS");
    return value;
}

bool SensorDevice::isOverheated() { return getStatus() & TS_STATUS_OVERHEAT; }
bool SensorDevice::isCoolingOn()  { return getStatus() & TS_STATUS_COOLING; }

void SensorDevice::setCooling(bool on) {
    __u32 value = on ? 1 : 0;
    doIoctl(TS_IOC_SET_COOLING, &value, "SET_COOLING");
}

void SensorDevice::reset() {
    doIoctl(TS_IOC_RESET, nullptr, "RESET");
}

tempsensor_stats SensorDevice::getStats() {
    tempsensor_stats stats{};
    doIoctl(TS_IOC_GET_STATS, &stats, "GET_STATS");
    return stats;
}
