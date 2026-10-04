#include "Types.h"

#include <ctime>
#include <cstdio>

const char* levelName(AlertLevel level) {
    switch (level) {
        case AlertLevel::Normal:   return "NORMAL";
        case AlertLevel::Warning:  return "WARNING";
        case AlertLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

std::string formatTime(Clock::time_point tp) {
    std::time_t t = Clock::to_time_t(tp);
    std::tm local{};
    localtime_r(&t, &local);              // thread-safe version of localtime
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local);
    return buf;
}

std::string formatTemp(double celsius) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.2f", celsius);
    return buf;
}
