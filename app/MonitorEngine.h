/*
 * MonitorEngine.h - the heart of the application.
 *
 * Runs a background THREAD that repeats, every interval:
 *     sense  : read the temperature from the sensor
 *     decide : classify it (AlertManager)
 *     act    : switch the cooling fan if needed (CoolingController)
 *     record : store it (TemperatureHistory) and log it (LoggerProcess)
 *
 * The menu (main thread) stays responsive while this thread works.
 */
#ifndef MONITORENGINE_H
#define MONITORENGINE_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "AlertManager.h"
#include "CoolingController.h"
#include "ITemperatureSource.h"
#include "LoggerProcess.h"
#include "TemperatureHistory.h"

class MonitorEngine {
public:
    enum class State { Idle, Running, Error };

    MonitorEngine(ITemperatureSource& source, AlertManager& alerts,
                  CoolingController& cooling, TemperatureHistory& history,
                  LoggerProcess* logger);
    ~MonitorEngine();

    void start();
    void stop();
    bool isRunning() const { return running_; }
    State state() const;
    std::string lastError() const;

    void setInterval(unsigned ms);      // also tells the driver
    unsigned interval() const { return intervalMs_; }

private:
    void run();                         // the thread function
    void log(const std::string& msg);

    ITemperatureSource& source_;
    AlertManager& alerts_;
    CoolingController& cooling_;
    TemperatureHistory& history_;
    LoggerProcess* logger_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<unsigned> intervalMs_;
    std::mutex waitMutex_;
    std::condition_variable wakeUp_;    // lets stop() interrupt the sleep immediately

    mutable std::mutex stateMutex_;
    State state_ = State::Idle;
    std::string lastError_;
};

const char* stateName(MonitorEngine::State s);

#endif
