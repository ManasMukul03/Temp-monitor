#include "CoolingController.h"

#include <cstdio>

std::optional<std::string> CoolingController::update(AlertLevel level, ITemperatureSource& source) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!auto_)
        return std::nullopt;

    if (level == AlertLevel::Critical && !fanOn_) {
        source.setCooling(true);                       // -> ioctl(TS_IOC_SET_COOLING, 1)
        fanOn_ = true;
        activations_++;
        fanOnSince_ = std::chrono::steady_clock::now();
        return std::string("FAN ON (automatic) - critical temperature");
    }

    if (level == AlertLevel::Normal && fanOn_) {
        source.setCooling(false);                      // -> ioctl(TS_IOC_SET_COOLING, 0)
        fanOn_ = false;
        double secs = std::chrono::duration<double>(
                          std::chrono::steady_clock::now() - fanOnSince_).count();
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.1f", secs);
        return std::string("FAN OFF (automatic) - temperature back to normal, recovered in ")
               + buf + " s";
    }
    return std::nullopt;
}

std::string CoolingController::manual(bool on, ITemperatureSource& source) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto_ = false;
    source.setCooling(on);
    fanOn_ = on;
    return std::string("FAN ") + (on ? "ON" : "OFF") + " (manual)";
}

void CoolingController::setAuto(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto_ = enabled;
}

bool CoolingController::isAuto() const    { std::lock_guard<std::mutex> l(mutex_); return auto_; }
bool CoolingController::isFanOn() const   { std::lock_guard<std::mutex> l(mutex_); return fanOn_; }
int CoolingController::activations() const { std::lock_guard<std::mutex> l(mutex_); return activations_; }
