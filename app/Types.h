/*
 * Types.h - basic types shared by the whole application.
 */
#ifndef TYPES_H
#define TYPES_H

#include <chrono>
#include <string>

// The three alert levels of the system
enum class AlertLevel { Normal, Warning, Critical };

using Clock = std::chrono::system_clock;

// One temperature sample
struct Reading {
    Clock::time_point time;
    double celsius;
    AlertLevel level;
};

const char* levelName(AlertLevel level);            // "NORMAL", "WARNING", "CRITICAL"
std::string formatTime(Clock::time_point tp);        // "2026-10-04 08:15:02"
std::string formatTemp(double celsius);              // "36.25"

#endif
