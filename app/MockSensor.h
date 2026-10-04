/*
 * MockSensor.h - fake sensor for unit tests.
 * Returns a fixed list of temperatures, one per call (the last one repeats).
 * Lets the application logic be tested without the kernel driver.
 */
#ifndef MOCKSENSOR_H
#define MOCKSENSOR_H

#include <vector>
#include "ITemperatureSource.h"

class MockSensor : public ITemperatureSource {
public:
    explicit MockSensor(std::vector<double> values) : values_(std::move(values)) {}

    double readCelsius() override {
        double v = values_[index_];
        if (index_ + 1 < values_.size()) index_++;
        reads_++;
        return v;
    }
    void setInterval(unsigned ms) override { interval_ = ms; }
    unsigned getInterval() override { return interval_; }
    bool isOverheated() override { return values_[index_] > 60.0; }
    void setCooling(bool on) override { cooling_ = on; if (on) coolingOnCount_++; }
    bool isCoolingOn() override { return cooling_; }
    void reset() override { index_ = 0; cooling_ = false; }

    int reads() const { return reads_; }
    int coolingOnCount() const { return coolingOnCount_; }

private:
    std::vector<double> values_;
    size_t index_ = 0;
    unsigned interval_ = 20;
    bool cooling_ = false;
    int reads_ = 0;
    int coolingOnCount_ = 0;
};

#endif
