#include "TemperatureHistory.h"

#include <algorithm>

TemperatureHistory::TemperatureHistory(size_t capacity) : capacity_(capacity) {}

void TemperatureHistory::add(const Reading& r) {
    std::lock_guard<std::mutex> lock(mutex_);
    readings_.push_back(r);
    if (readings_.size() > capacity_)
        readings_.pop_front();               // deque: O(1) removal at the front
    total_++;
    counts_[r.level]++;
}

HistoryStats TemperatureHistory::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    HistoryStats s;
    if (readings_.empty()) return s;
    s.count = readings_.size();
    s.min = s.max = readings_.front().celsius;
    double sum = 0;
    for (const Reading& r : readings_) {
        s.min = std::min(s.min, r.celsius);
        s.max = std::max(s.max, r.celsius);
        sum += r.celsius;
    }
    s.average = sum / readings_.size();
    return s;
}

std::vector<Reading> TemperatureHistory::recent(size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t start = readings_.size() > n ? readings_.size() - n : 0;
    return std::vector<Reading>(readings_.begin() + start, readings_.end());
}

std::vector<Reading> TemperatureHistory::all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::vector<Reading>(readings_.begin(), readings_.end());
}

std::map<AlertLevel, int> TemperatureHistory::levelCounts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return counts_;
}

size_t TemperatureHistory::totalReadings() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_;
}

bool TemperatureHistory::latest(Reading& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (readings_.empty()) return false;
    out = readings_.back();
    return true;
}

void TemperatureHistory::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    readings_.clear();
    counts_.clear();
    total_ = 0;
}
