/*
 * main.cpp - Virtual Temperature Monitoring & Protection System
 *
 * Console application that:
 *   - opens /dev/tempsensor (the kernel driver)
 *   - starts a logger child process (fork + pipe)
 *   - starts the monitoring thread (sense -> decide -> act -> record)
 *   - offers a menu for live view, statistics, fan control, reports
 *   - shuts down safely on Ctrl+C (SIGINT)
 *
 * Build:  cd app && make        Run:  ./tempmon   (driver must be loaded)
 */

#include <cerrno>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <poll.h>
#include <unistd.h>

#include "AlertManager.h"
#include "CoolingController.h"
#include "LoggerProcess.h"
#include "MonitorEngine.h"
#include "ReportExporter.h"
#include "SensorDevice.h"
#include "SystemInfo.h"
#include "TemperatureHistory.h"

// ------------------------------------------------------------------
// Signal handling: Ctrl+C only sets a flag. The main loop sees the
// flag and shuts everything down in the correct order.
// ------------------------------------------------------------------
static volatile sig_atomic_t g_stopRequested = 0;
static volatile sig_atomic_t g_signalReceived = 0;

static void onSignal(int) {
    g_signalReceived = 1;
    g_stopRequested = 1;
}

static void installSignalHandlers() {
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                 // no SA_RESTART: a waiting poll() returns at once
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    std::signal(SIGPIPE, SIG_IGN);   // never die because the logger pipe closed
}

// ------------------------------------------------------------------
// Input helpers. poll() waits for keyboard input with a timeout, so we
// can notice Ctrl+C even while waiting for the user to type.
// ------------------------------------------------------------------
static bool inputReady(int timeoutMs) {
    struct pollfd p{STDIN_FILENO, POLLIN, 0};
    int r = poll(&p, 1, timeoutMs);
    return r > 0;
}

static bool readLine(std::string& out) {
    while (!g_stopRequested) {
        if (inputReady(200)) {
            if (!std::getline(std::cin, out)) {   // end of input
                g_stopRequested = 1;
                return false;
            }
            return true;
        }
    }
    return false;
}

static bool readNumber(const std::string& prompt, double lo, double hi, double& value) {
    while (!g_stopRequested) {
        std::cout << prompt << std::flush;
        std::string line;
        if (!readLine(line)) return false;
        try {
            size_t used = 0;
            value = std::stod(line, &used);
            if (used == line.size() && value >= lo && value <= hi) return true;
        } catch (...) {}
        std::cout << "  Please enter a number between " << lo << " and " << hi << ".\n";
    }
    return false;
}

// ------------------------------------------------------------------
// Display helpers (ANSI colours: green / yellow / red)
// ------------------------------------------------------------------
static const char* colourFor(AlertLevel level) {
    switch (level) {
        case AlertLevel::Normal:   return "\033[32m";
        case AlertLevel::Warning:  return "\033[33m";
        case AlertLevel::Critical: return "\033[31;1m";
    }
    return "";
}
static const char* RESET = "\033[0m";

static void printReading(const Reading& r, bool fanOn) {
    std::cout << "  [" << formatTime(r.time) << "]  "
              << colourFor(r.level) << std::setw(6) << formatTemp(r.celsius) << " C  "
              << std::left << std::setw(8) << levelName(r.level) << std::right << RESET
              << "  fan: " << (fanOn ? "ON " : "OFF") << "\n";
}

// ------------------------------------------------------------------
// The application: owns all the parts and the menu
// ------------------------------------------------------------------
class App {
public:
    App(SensorDevice& sensor, LoggerProcess& logger)
        : sensor_(sensor), logger_(logger),
          engine_(sensor_, alerts_, cooling_, history_, &logger_) {}

    int run();

private:
    void printMenu();
    void liveView();
    void showStatistics();
    void showRecent();
    void changeThresholds();
    void fanControl();
    void changeInterval();
    void driverStatus();
    void systemInformation();
    void exportReport();
    void shutdown();

    SensorDevice& sensor_;
    LoggerProcess& logger_;
    AlertManager alerts_;
    CoolingController cooling_;
    TemperatureHistory history_;
    MonitorEngine engine_;
};

void App::printMenu() {
    Reading last{};
    bool hasLast = history_.latest(last);
    std::cout << "\n========== VIRTUAL TEMPERATURE MONITOR ==========\n"
              << "  Monitor: " << stateName(engine_.state())
              << "   Fan: " << (cooling_.isFanOn() ? "ON" : "OFF")
              << (cooling_.isAuto() ? " (auto)" : " (manual)");
    if (hasLast)
        std::cout << "   Last: " << colourFor(last.level) << formatTemp(last.celsius)
                  << " C " << levelName(last.level) << RESET;
    std::cout << "\n-------------------------------------------------\n"
              << "  1. Start monitoring\n"
              << "  2. Stop monitoring\n"
              << "  3. Live view\n"
              << "  4. Statistics\n"
              << "  5. Recent readings\n"
              << "  6. Alert thresholds\n"
              << "  7. Cooling fan control\n"
              << "  8. Sampling interval\n"
              << "  9. Driver status (registers)\n"
              << " 10. System information\n"
              << " 11. Export CSV report\n"
              << "  0. Exit\n"
              << "Choice: " << std::flush;
}

int App::run() {
    engine_.start();                      // monitoring starts automatically
    while (!g_stopRequested) {
        if (engine_.state() == MonitorEngine::State::Error)
            std::cout << "\n  Monitor stopped because of an error: " << engine_.lastError() << "\n";
        printMenu();
        std::string choice;
        if (!readLine(choice)) break;

        if (choice == "1") {
            engine_.start();
            std::cout << "  Monitoring started.\n";
        } else if (choice == "2") {
            engine_.stop();
            std::cout << "  Monitoring stopped.\n";
        } else if (choice == "3") liveView();
        else if (choice == "4") showStatistics();
        else if (choice == "5") showRecent();
        else if (choice == "6") changeThresholds();
        else if (choice == "7") fanControl();
        else if (choice == "8") changeInterval();
        else if (choice == "9") driverStatus();
        else if (choice == "10") systemInformation();
        else if (choice == "11") exportReport();
        else if (choice == "0") break;
        else std::cout << "  Unknown option.\n";
    }
    shutdown();
    return 0;
}

// Prints every new reading until the user presses Enter (or Ctrl+C)
void App::liveView() {
    if (!engine_.isRunning()) {
        std::cout << "  Monitoring is not running. Start it first (option 1).\n";
        return;
    }
    std::cout << "\n  LIVE VIEW  (warning >= " << alerts_.warningLimit()
              << " C, critical > " << alerts_.criticalLimit() << " C)  - press Enter to return\n";
    size_t shown = history_.totalReadings();
    while (!g_stopRequested && engine_.isRunning()) {
        if (inputReady(100)) {
            std::string ignore;
            std::getline(std::cin, ignore);
            break;
        }
        size_t now = history_.totalReadings();
        if (now != shown) {
            Reading r{};
            if (history_.latest(r)) printReading(r, cooling_.isFanOn());
            shown = now;
        }
    }
}

void App::showStatistics() {
    HistoryStats s = history_.stats();
    auto counts = history_.levelCounts();
    std::cout << "\n  STATISTICS (last " << s.count << " readings stored)\n";
    if (s.count == 0) {
        std::cout << "  No readings yet.\n";
        return;
    }
    std::cout << "  Minimum      : " << formatTemp(s.min) << " C\n"
              << "  Maximum      : " << formatTemp(s.max) << " C\n"
              << "  Average      : " << formatTemp(s.average) << " C\n"
              << "  Total read   : " << history_.totalReadings() << "\n"
              << "  Normal       : " << counts[AlertLevel::Normal] << "\n"
              << "  Warning      : " << counts[AlertLevel::Warning] << "\n"
              << "  Critical     : " << counts[AlertLevel::Critical] << "\n"
              << "  Fan activations (automatic): " << cooling_.activations() << "\n";
}

void App::showRecent() {
    auto list = history_.recent(10);
    std::cout << "\n  LAST " << list.size() << " READINGS\n";
    for (const Reading& r : list) printReading(r, cooling_.isFanOn());
}

void App::changeThresholds() {
    std::cout << "\n  Current: warning >= " << alerts_.warningLimit()
              << " C, critical > " << alerts_.criticalLimit() << " C\n";
    double warn, crit;
    if (!readNumber("  New warning limit (C): ", -40, 125, warn)) return;
    if (!readNumber("  New critical limit (C): ", -40, 125, crit)) return;
    if (alerts_.setThresholds(warn, crit)) {
        logger_.log("THRESHOLDS warning=" + formatTemp(warn) + " critical=" + formatTemp(crit));
        std::cout << "  Thresholds updated.\n";
    } else {
        std::cout << "  Invalid: warning must be lower than critical.\n";
    }
}

void App::fanControl() {
    std::cout << "\n  Fan is " << (cooling_.isFanOn() ? "ON" : "OFF")
              << ", mode " << (cooling_.isAuto() ? "AUTOMATIC" : "MANUAL") << "\n"
              << "  1. Automatic mode (on at Critical, off at Normal)\n"
              << "  2. Manual: fan ON\n"
              << "  3. Manual: fan OFF\n"
              << "  Choice: " << std::flush;
    std::string c;
    if (!readLine(c)) return;
    try {
        if (c == "1") {
            cooling_.setAuto(true);
            logger_.log("FAN mode automatic");
            std::cout << "  Automatic cooling enabled.\n";
        } else if (c == "2" || c == "3") {
            logger_.log(cooling_.manual(c == "2", sensor_));
            std::cout << "  Fan switched " << (c == "2" ? "ON" : "OFF") << " (manual mode).\n";
        }
    } catch (const std::exception& e) {
        std::cout << "  Error: " << e.what() << "\n";
    }
}

void App::changeInterval() {
    std::cout << "\n  Current interval: " << engine_.interval() << " ms\n";
    double ms;
    if (!readNumber("  New interval in ms (100-10000): ", 100, 10000, ms)) return;
    try {
        engine_.setInterval(static_cast<unsigned>(ms));
        std::cout << "  Interval set to " << static_cast<unsigned>(ms) << " ms.\n";
    } catch (const std::exception& e) {
        std::cout << "  Error: " << e.what() << "\n";
    }
}

void App::driverStatus() {
    try {
        unsigned st = sensor_.getStatus();
        tempsensor_stats ds = sensor_.getStats();
        std::cout << "\n  DRIVER STATUS (" << sensor_.path() << ")\n"
                  << "  STATUS register : 0x" << std::hex << st << std::dec << "\n"
                  << "    data_ready=" << !!(st & TS_STATUS_DATA_READY)
                  << " overheat=" << !!(st & TS_STATUS_OVERHEAT)
                  << " enabled=" << !!(st & TS_STATUS_ENABLED)
                  << " cooling=" << !!(st & TS_STATUS_COOLING) << "\n"
                  << "  INTERVAL        : " << sensor_.getInterval() << " ms\n"
                  << "  Driver stats    : min " << formatTemp(ds.min_mc / 1000.0)
                  << " C, max " << formatTemp(ds.max_mc / 1000.0)
                  << " C, readings " << ds.count << "\n";
    } catch (const std::exception& e) {
        std::cout << "  Error: " << e.what() << "\n";
    }
}

void App::systemInformation() {
    SystemInfo info;
    if (!info.load()) {
        std::cout << "  Could not read /proc.\n";
        return;
    }
    std::cout << "\n  SYSTEM INFORMATION\n"
              << "  Kernel        : " << info.kernel << "\n"
              << "  Architecture  : " << info.machine << " (" << info.wordBits << "-bit)\n"
              << "  CPU model     : " << info.cpuModel << "\n"
              << "  CPU cores     : " << info.cpuCores << "\n"
              << "  Cache size    : " << info.cacheSize << "\n"
              << "  Address sizes : " << info.addressSizes << "\n"
              << "  Byte order    : " << info.byteOrder << "\n"
              << "  Page size     : " << info.pageSize << " bytes\n"
              << "  Memory total  : " << info.memTotalKb / 1024 << " MB\n"
              << "  Memory free   : " << info.memAvailableKb / 1024 << " MB\n";
}

void App::exportReport() {
    const std::string path = "readings.csv";
    long rows = ReportExporter::exportCsv(history_.all(), path);
    if (rows < 0) {
        std::cout << "  Could not write " << path << "\n";
        return;
    }
    logger_.log("REPORT exported " + std::to_string(rows) + " rows to " + path);
    std::cout << "  Exported " << rows << " readings to " << path << "\n";
}

// Correct shutdown order: thread -> fan -> logger
void App::shutdown() {
    if (g_signalReceived)
        std::cout << "\n\n  Ctrl+C received - shutting down safely...\n";
    else
        std::cout << "\n  Shutting down safely...\n";
    engine_.stop();
    std::cout << "  Monitoring thread stopped.\n";
    if (cooling_.isFanOn()) {
        try { sensor_.setCooling(false); } catch (...) {}
        logger_.log("FAN OFF (shutdown)");
        std::cout << "  Cooling fan switched off.\n";
    }
    logger_.log("APP stopped, total readings " + std::to_string(history_.totalReadings()));
    logger_.stop();
    std::cout << "  Logger process finished, log saved to tempmon.log\n"
              << "  Goodbye.\n";
}

int main() {
    installSignalHandlers();

    // 1. Logger first: fork() before any thread exists
    LoggerProcess logger;
    if (!logger.start("tempmon.log")) {
        std::cerr << "Could not start the logger process.\n";
        return 1;
    }
    logger.log("APP started");

    // 2. Open the device
    std::unique_ptr<SensorDevice> sensor;
    try {
        sensor = std::make_unique<SensorDevice>();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                  << "Is the driver loaded?  cd driver && sudo insmod tempsensor.ko\n";
        logger.log(std::string("ERROR ") + e.what());
        logger.stop();
        return 1;
    }
    std::cout << "Connected to " << sensor->path()
              << " (logger process pid " << logger.childPid() << ")\n";

    // 3. Run the menu
    App app(*sensor, logger);
    return app.run();
}
