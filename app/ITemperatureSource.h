/*
 * ITemperatureSource.h - abstract interface for anything that provides
 * temperatures and a cooling fan.
 *
 *   SensorDevice : real implementation, talks to /dev/tempsensor
 *   MockSensor   : fake implementation with fixed values, used in tests
 *
 * The rest of the application only uses this interface (polymorphism),
 * so a different sensor can be plugged in without changing it.
 */
#ifndef ITEMPERATURESOURCE_H
#define ITEMPERATURESOURCE_H

class ITemperatureSource {
public:
    virtual ~ITemperatureSource() = default;

    virtual double readCelsius() = 0;          // throws std::runtime_error on failure
    virtual void setInterval(unsigned ms) = 0;
    virtual unsigned getInterval() = 0;
    virtual bool isOverheated() = 0;
    virtual void setCooling(bool on) = 0;
    virtual bool isCoolingOn() = 0;
    virtual void reset() = 0;
};

#endif
