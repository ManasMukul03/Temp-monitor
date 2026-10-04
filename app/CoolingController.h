/*
 * CoolingController.h - automatic overheating protection.
 *
 *   Automatic mode: fan ON when a reading is Critical,
 *                   fan OFF only when the reading is back to Normal.
 *   The gap between the two (hysteresis) stops the fan from
 *   switching on and off repeatedly around one limit.
 *
 *   Manual mode: the user switches the fan; automatic control is off.
 */
#ifndef COOLINGCONTROLLER_H
#define COOLINGCONTROLLER_H

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include "ITemperatureSource.h"
#include "Types.h"

class CoolingController {
public:
    // Called for every reading. Returns a log message if the fan changed.
    std::optional<std::string> update(AlertLevel level, ITemperatureSource& source);

    // Manual control (turns automatic mode off). Returns a log message.
    std::string manual(bool on, ITemperatureSource& source);
    void setAuto(bool enabled);

    bool isAuto() const;
    bool isFanOn() const;
    int activations() const;        // how many times the fan was switched on automatically

private:
    mutable std::mutex mutex_;
    bool auto_ = true;
    bool fanOn_ = false;
    int activations_ = 0;
    std::chrono::steady_clock::time_point fanOnSince_;
};

#endif
