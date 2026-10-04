/*
 * app_test.cpp - unit and integration tests for the C++ application.
 * Uses MockSensor, so it runs WITHOUT the kernel driver.
 *
 * Run:  cd app && make test
 */
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

#include "AlertManager.h"
#include "CoolingController.h"
#include "LoggerProcess.h"
#include "MockSensor.h"
#include "MonitorEngine.h"
#include "ReportExporter.h"
#include "SystemInfo.h"
#include "TemperatureHistory.h"

static int passed = 0, failed = 0;

static void check(bool ok, const char* name) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    ok ? passed++ : failed++;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static Reading make(double c, AlertLevel l) { return {Clock::now(), c, l}; }

int main() {
    std::printf("=== Application unit & integration tests ===\n");

    // ---------------- AlertManager ----------------
    AlertManager alerts;   // warning 45, critical 60
    check(alerts.classify(30.0) == AlertLevel::Normal,    "U1 30 C is NORMAL");
    check(alerts.classify(45.0) == AlertLevel::Warning,   "U2 45 C is WARNING (boundary)");
    check(alerts.classify(60.0) == AlertLevel::Warning,   "U3 60 C is WARNING (boundary)");
    check(alerts.classify(60.1) == AlertLevel::Critical,  "U4 60.1 C is CRITICAL");
    check(!alerts.setThresholds(70, 50),                   "U5 invalid thresholds rejected");
    check(alerts.setThresholds(40, 55) && alerts.classify(56) == AlertLevel::Critical,
          "U6 new thresholds applied");
    alerts.setThresholds(45, 60);

    // ---------------- CoolingController ----------------
    {
        MockSensor sensor({30});
        CoolingController cooling;
        auto e1 = cooling.update(AlertLevel::Critical, sensor);
        check(e1.has_value() && sensor.isCoolingOn(), "U7 CRITICAL turns fan ON");
        auto e2 = cooling.update(AlertLevel::Warning, sensor);
        check(!e2.has_value() && sensor.isCoolingOn(), "U8 WARNING keeps fan ON (hysteresis)");
        auto e3 = cooling.update(AlertLevel::Normal, sensor);
        check(e3.has_value() && !sensor.isCoolingOn(), "U9 NORMAL turns fan OFF");
        cooling.manual(true, sensor);
        cooling.update(AlertLevel::Normal, sensor);
        check(!cooling.isAuto() && sensor.isCoolingOn(), "U10 manual mode ignores automatic rules");
    }

    // ---------------- TemperatureHistory ----------------
    {
        TemperatureHistory h(3);
        h.add(make(30, AlertLevel::Normal));
        h.add(make(50, AlertLevel::Warning));
        h.add(make(70, AlertLevel::Critical));
        HistoryStats s = h.stats();
        check(s.count == 3 && s.min == 30 && s.max == 70 && s.average == 50,
              "U11 min / max / average correct");
        h.add(make(40, AlertLevel::Normal));
        check(h.stats().count == 3 && h.recent(1)[0].celsius == 40 && h.totalReadings() == 4,
              "U12 capacity limit drops oldest reading");
        check(h.levelCounts()[AlertLevel::Normal] == 2, "U13 level counts");
    }

    // ---------------- ReportExporter ----------------
    {
        std::vector<Reading> rows = {make(30, AlertLevel::Normal), make(65, AlertLevel::Critical)};
        long n = ReportExporter::exportCsv(rows, "/tmp/tm_test.csv");
        std::string csv = readFile("/tmp/tm_test.csv");
        check(n == 2 && csv.find("timestamp,temperature_c,level") == 0 &&
              csv.find("65.00,CRITICAL") != std::string::npos, "U14 CSV report written");
        std::remove("/tmp/tm_test.csv");
    }

    // ---------------- SystemInfo ----------------
    {
        SystemInfo info;
        check(info.load() && info.cpuCores > 0 && info.memTotalKb > 0 && !info.machine.empty(),
              "U15 system info read from /proc and uname()");
    }

    // ---------------- LoggerProcess (fork + pipe) ----------------
    {
        const char* path = "/tmp/tm_test.log";
        std::remove(path);
        LoggerProcess logger;
        bool started = logger.start(path);
        pid_t child = logger.childPid();
        logger.log("hello from parent");
        logger.stop();
        std::string log = readFile(path);
        check(started && child > 0 && child != getpid(), "I1 logger runs as a separate child process");
        check(log.find("hello from parent") != std::string::npos &&
              log.find("stopped") != std::string::npos, "I2 messages travel through the pipe to the file");
        std::remove(path);
    }

    // ---------------- MonitorEngine with MockSensor (integration) ----------------
    {
        // Heats up past 60 C, then cools down below 45 C
        MockSensor sensor({40, 50, 58, 63, 66, 61, 55, 48, 43, 40});
        sensor.setInterval(20);
        AlertManager a;
        CoolingController c;
        TemperatureHistory h;
        const char* path = "/tmp/tm_engine.log";
        std::remove(path);
        LoggerProcess logger;
        logger.start(path);

        MonitorEngine engine(sensor, a, c, h, &logger);
        engine.start();
        check(engine.isRunning(), "I3 monitor thread starts");
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        engine.stop();
        logger.stop();
        std::string log = readFile(path);

        check(!engine.isRunning() && h.totalReadings() >= 10, "I4 thread reads sensor repeatedly, stops cleanly");
        check(sensor.coolingOnCount() == 1 && !sensor.isCoolingOn(),
              "I5 overheating: fan turned ON once and OFF after recovery");
        check(log.find("FAN ON (automatic)") != std::string::npos &&
              log.find("FAN OFF (automatic)") != std::string::npos &&
              log.find("ALERT WARNING -> CRITICAL") != std::string::npos,
              "I6 alerts and fan events written to the log");
        std::remove(path);
    }

    std::printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
