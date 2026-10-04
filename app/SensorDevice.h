/*
 * SensorDevice.h - talks to the tempsensor driver through system calls.
 *
 * open()  in the constructor, close() in the destructor (RAII):
 * the device is always closed, even if an exception happens.
 */
#ifndef SENSORDEVICE_H
#define SENSORDEVICE_H

#include <string>
#include "ITemperatureSource.h"
#include "tempsensor_ioctl.h"

class SensorDevice : public ITemperatureSource {
public:
    explicit SensorDevice(const std::string& path = TEMPSENSOR_DEVICE_PATH);
    ~SensorDevice() override;

    SensorDevice(const SensorDevice&) = delete;             // one owner of the file descriptor
    SensorDevice& operator=(const SensorDevice&) = delete;

    double readCelsius() override;
    void setInterval(unsigned ms) override;
    unsigned getInterval() override;
    bool isOverheated() override;
    void setCooling(bool on) override;
    bool isCoolingOn() override;
    void reset() override;

    // driver-specific extras
    unsigned getStatus();
    tempsensor_stats getStats();
    const std::string& path() const { return path_; }

private:
    void doIoctl(unsigned long cmd, void* arg, const char* name);

    std::string path_;
    int fd_ = -1;      // file descriptor returned by open()
};

#endif
