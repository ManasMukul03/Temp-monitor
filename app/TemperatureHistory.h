/*
 * TemperatureHistory.h - stores recent readings and calculates statistics.
 * Thread-safe: the monitor thread adds readings while the menu reads them.
 */
#ifndef TEMPERATUREHISTORY_H
#define TEMPERATUREHISTORY_H

#include <deque>
#include <map>
#include <mutex>
#include <vector>
#include "Types.h"

struct HistoryStats {
    size_t count = 0;
    double min = 0, max = 0, average = 0;
};

class TemperatureHistory {
public:
    explicit TemperatureHistory(size_t capacity = 1000);

    void add(const Reading& r);
    HistoryStats stats() const;
    std::vector<Reading> recent(size_t n) const;      // newest last
    std::vector<Reading> all() const;
    std::map<AlertLevel, int> levelCounts() const;    // all-time counts
    size_t totalReadings() const;                     // all-time count
    bool latest(Reading& out) const;                  // false if empty
    void clear();

private:
    mutable std::mutex mutex_;
    std::deque<Reading> readings_;    // fixed size: oldest removed when full
    size_t capacity_;
    size_t total_ = 0;
    std::map<AlertLevel, int> counts_;
};

#endif
