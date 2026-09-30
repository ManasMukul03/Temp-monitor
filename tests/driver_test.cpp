/*
 * driver_test.cpp - checks every operation of /dev/tempsensor from user space.
 *
 * Build:  g++ -std=c++17 -Wall -I../driver driver_test.cpp -o driver_test
 * Run:    ./driver_test          (driver must be loaded first)
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "tempsensor_ioctl.h"

static int passed = 0, failed = 0;

static void check(bool ok, const char* name) {
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    ok ? passed++ : failed++;
}

// Reads the current temperature (milli-degC). Returns false on error.
static bool readTemp(int fd, int& valueMc) {
    char buf[32] = {0};
    ssize_t n = pread(fd, buf, sizeof(buf) - 1, 0);   // offset 0 = always fresh value
    if (n <= 0) return false;
    valueMc = std::atoi(buf);
    return true;
}

int main() {
    std::printf("=== tempsensor driver test ===\n");

    int fd = open(TEMPSENSOR_DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        std::printf("Cannot open %s: %s\nIs the driver loaded? (sudo insmod tempsensor.ko)\n",
                    TEMPSENSOR_DEVICE_PATH, std::strerror(errno));
        return 1;
    }
    check(true, "T1 open /dev/tempsensor");

    // T2: reading returns a value within sensor limits
    int temp = 0;
    bool ok = readTemp(fd, temp);
    check(ok && temp >= -40000 && temp <= 125000, "T2 read returns a valid temperature");
    std::printf("      current temperature: %.3f C\n", temp / 1000.0);

    // T3: get interval
    __u32 interval = 0;
    check(ioctl(fd, TS_IOC_GET_INTERVAL, &interval) == 0 && interval >= TS_MIN_INTERVAL_MS,
          "T3 GET_INTERVAL");
    std::printf("      interval: %u ms\n", interval);

    // T4: set a valid interval
    __u32 newInterval = 200;
    ioctl(fd, TS_IOC_SET_INTERVAL, &newInterval);
    ioctl(fd, TS_IOC_GET_INTERVAL, &interval);
    check(interval == 200, "T4 SET_INTERVAL 200 ms");

    // T5: invalid interval is rejected with EINVAL
    __u32 bad = 5;
    int rc = ioctl(fd, TS_IOC_SET_INTERVAL, &bad);
    check(rc == -1 && errno == EINVAL, "T5 SET_INTERVAL 5 ms rejected (EINVAL)");

    // T6: reset clears statistics
    ioctl(fd, TS_IOC_RESET);
    tempsensor_stats st{};
    ioctl(fd, TS_IOC_GET_STATS, &st);
    check(st.count <= 1, "T6 RESET clears statistics");

    // T7: new readings arrive over time and stats are consistent
    std::printf("      sampling for 2 seconds...\n");
    for (int i = 0; i < 5; i++) {
        usleep(400 * 1000);
        if (readTemp(fd, temp)) std::printf("      reading %d: %.3f C\n", i + 1, temp / 1000.0);
    }
    ioctl(fd, TS_IOC_GET_STATS, &st);
    check(st.count >= 5 && st.min_mc <= st.max_mc, "T7 readings counted, min <= max");
    std::printf("      stats: min %.3f  max %.3f  count %u\n",
                st.min_mc / 1000.0, st.max_mc / 1000.0, st.count);

    // T8: status register shows sensor enabled
    __u32 status = 0;
    ioctl(fd, TS_IOC_GET_STATUS, &status);
    check(status & TS_STATUS_ENABLED, "T8 STATUS shows ENABLED");
    std::printf("      status: 0x%x (overheat=%d)\n", status, (status & TS_STATUS_OVERHEAT) ? 1 : 0);

    // T9: disable stops new readings
    __u32 off = 0, on = 1;
    ioctl(fd, TS_IOC_SET_ENABLE, &off);
    tempsensor_stats before{}, after{};
    ioctl(fd, TS_IOC_GET_STATS, &before);
    usleep(600 * 1000);
    ioctl(fd, TS_IOC_GET_STATS, &after);
    check(after.count == before.count, "T9 SET_ENABLE 0 stops sampling");
    ioctl(fd, TS_IOC_SET_ENABLE, &on);

    // T11: cooling fan brings the temperature down (overheat protection)
    __u32 fast = 100, fanOn = 1, fanOff = 0;
    ioctl(fd, TS_IOC_SET_INTERVAL, &fast);
    ioctl(fd, TS_IOC_SET_COOLING, &fanOn);
    std::printf("      cooling fan ON, waiting 4 seconds...\n");
    usleep(4000 * 1000);
    readTemp(fd, temp);
    ioctl(fd, TS_IOC_GET_STATUS, &status);
    std::printf("      temperature with fan on: %.3f C\n", temp / 1000.0);
    check((status & TS_STATUS_COOLING) && temp < 40000, "T11 cooling fan brings temperature below 40 C");
    ioctl(fd, TS_IOC_SET_COOLING, &fanOff);

    // T10: unknown ioctl command returns ENOTTY
    rc = ioctl(fd, _IO(TEMPSENSOR_MAGIC, 99));
    check(rc == -1 && errno == ENOTTY, "T10 unknown ioctl rejected (ENOTTY)");

    // restore default interval
    __u32 def = 1000;
    ioctl(fd, TS_IOC_SET_INTERVAL, &def);
    close(fd);

    std::printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
