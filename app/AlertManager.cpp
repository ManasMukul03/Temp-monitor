#include "AlertManager.h"

AlertManager::AlertManager(double warning, double critical)
    : warning_(warning), critical_(critical) {}

AlertLevel AlertManager::classify(double celsius) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (celsius > critical_) return AlertLevel::Critical;
    if (celsius >= warning_) return AlertLevel::Warning;
    return AlertLevel::Normal;
}

bool AlertManager::setThresholds(double warning, double critical) {
    if (warning >= critical || warning < -40 || critical > 125)
        return false;
    std::lock_guard<std::mutex> lock(mutex_);
    warning_ = warning;
    critical_ = critical;
    return true;
}

double AlertManager::warningLimit() const  { std::lock_guard<std::mutex> l(mutex_); return warning_; }
double AlertManager::criticalLimit() const { std::lock_guard<std::mutex> l(mutex_); return critical_; }
