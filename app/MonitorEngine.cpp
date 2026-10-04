#include "MonitorEngine.h"

#include <chrono>
#include <exception>

const char* stateName(MonitorEngine::State s) {
    switch (s) {
        case MonitorEngine::State::Idle:    return "IDLE";
        case MonitorEngine::State::Running: return "RUNNING";
        case MonitorEngine::State::Error:   return "ERROR";
    }
    return "?";
}

MonitorEngine::MonitorEngine(ITemperatureSource& source, AlertManager& alerts,
                             CoolingController& cooling, TemperatureHistory& history,
                             LoggerProcess* logger)
    : source_(source), alerts_(alerts), cooling_(cooling), history_(history),
      logger_(logger), intervalMs_(source.getInterval()) {}

MonitorEngine::~MonitorEngine() { stop(); }

void MonitorEngine::log(const std::string& msg) {
    if (logger_) logger_->log(msg);
}

void MonitorEngine::start() {
    if (running_) return;
    if (worker_.joinable()) worker_.join();        // clean up after an earlier error
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        state_ = State::Running;
        lastError_.clear();
    }
    running_ = true;
    worker_ = std::thread(&MonitorEngine::run, this);   // start the background thread
    log("MONITOR started, interval " + std::to_string(intervalMs_) + " ms");
}

void MonitorEngine::stop() {
    {
        std::lock_guard<std::mutex> lock(waitMutex_);
        if (!running_ && !worker_.joinable()) return;
        running_ = false;
    }
    wakeUp_.notify_all();                         // wake the thread if it is sleeping
    if (worker_.joinable()) worker_.join();       // wait until the thread has finished
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (state_ == State::Running) {
        state_ = State::Idle;
        log("MONITOR stopped");
    }
}

void MonitorEngine::run() {
    bool first = true;
    AlertLevel previous = AlertLevel::Normal;

    while (running_) {
        try {
            // 1. SENSE
            double celsius = source_.readCelsius();
            // 2. DECIDE
            AlertLevel level = alerts_.classify(celsius);
            // 4. RECORD
            history_.add({Clock::now(), celsius, level});
            log("READING " + formatTemp(celsius) + " C " + levelName(level));
            if (!first && level != previous)
                log(std::string("ALERT ") + levelName(previous) + " -> " + levelName(level)
                    + " at " + formatTemp(celsius) + " C");
            previous = level;
            first = false;
            // 3. ACT
            if (auto event = cooling_.update(level, source_))
                log(*event);
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(stateMutex_);
            state_ = State::Error;
            lastError_ = e.what();
            log(std::string("ERROR ") + e.what());
            running_ = false;
            break;
        }

        // sleep for one interval, but wake up at once if stop() is called
        std::unique_lock<std::mutex> lock(waitMutex_);
        wakeUp_.wait_for(lock, std::chrono::milliseconds(intervalMs_.load()),
                         [this] { return !running_; });
    }
}

MonitorEngine::State MonitorEngine::state() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return state_;
}

std::string MonitorEngine::lastError() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return lastError_;
}

void MonitorEngine::setInterval(unsigned ms) {
    source_.setInterval(ms);              // driver validates the range (EINVAL if wrong)
    intervalMs_ = ms;
    log("INTERVAL set to " + std::to_string(ms) + " ms");
}
