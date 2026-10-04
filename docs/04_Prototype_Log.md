# Stage 4 – Initial Implementation & Prototype

**Project:** Virtual Temperature Monitoring & Protection System
**Author:** Manas Mukul
**Version:** 1.0 – 5 October 2026

---

## 1. Objective of this Stage

Implement the core modules designed in Stage 3, integrate them step by step, and demonstrate a working prototype in which the application monitors the kernel driver in real time and protects the device from overheating automatically.

## 2. Implementation Order (progressive integration)

The system was built from the bottom layer upwards, so that every new layer was placed on top of a layer that was already tested.

| Step | Component | Verified by |
|------|-----------|-------------|
| 1 | Shared interface header (`tempsensor_ioctl.h`) | Compiles in both kernel and user space |
| 2 | Kernel driver (`tempsensor.c`): char device, registers, timer, `read`, `ioctl`, `/proc` | Manual tests M1–M13, driver tests T1–T11 |
| 3 | `ITemperatureSource` interface + `MockSensor` | Unit tests with fixed values |
| 4 | `AlertManager`, `CoolingController`, `TemperatureHistory` | Unit tests U1–U13 |
| 5 | `LoggerProcess` (fork + pipe) | Integration tests I1–I2 |
| 6 | `MonitorEngine` thread connected to `MockSensor` | Integration tests I3–I6 |
| 7 | `SensorDevice` – real system calls on `/dev/tempsensor` | Application running against the loaded driver |
| 8 | `SystemInfo`, `ReportExporter`, console menu, Ctrl+C handling | System run on the VM |

## 3. Implemented Modules

### 3.1 Kernel space (C)

| File | Lines | Responsibility |
|------|-------|----------------|
| `driver/tempsensor_ioctl.h` | 59 | Register bits, ioctl command numbers, statistics structure (shared contract) |
| `driver/tempsensor.c` | 436 | Character device, simulated sensor and cooling fan, kernel timer, `read`, `ioctl`, `/proc/tempsensor`, spinlock |

### 3.2 User space (C++17)

| File | Lines | Responsibility |
|------|-------|----------------|
| `Types.h/.cpp` | 54 | `AlertLevel`, `Reading`, time and temperature formatting |
| `ITemperatureSource.h` | 27 | Abstract interface for any temperature source (polymorphism) |
| `SensorDevice.h/.cpp` | 112 | Wraps `open`, `pread`, `ioctl`, `close`; RAII closes the device automatically |
| `MockSensor.h` | 41 | Fake sensor with fixed values for tests |
| `AlertManager.h/.cpp` | 51 | Normal / Warning / Critical classification, configurable thresholds |
| `CoolingController.h/.cpp` | 88 | Automatic fan control with hysteresis, manual mode, recovery time |
| `TemperatureHistory.h/.cpp` | 105 | `std::deque` history (capacity 1000), min / max / average, level counts |
| `LoggerProcess.h/.cpp` | 122 | Logger child process created with `fork`, fed through a `pipe` |
| `MonitorEngine.h/.cpp` | 174 | Background `std::thread`: sense → decide → act → record |
| `SystemInfo.h/.cpp` | 81 | CPU, cache, memory, address sizes, byte order, page size from `/proc`, `uname`, `sysconf` |
| `ReportExporter.h/.cpp` | 29 | CSV report export |
| `main.cpp` | 394 | Console menu, live view, signal handling (SIGINT/SIGTERM), safe shutdown |
| `Makefile` | – | `make` builds `tempmon`, `make test` runs the tests |

**Total source code:** driver ≈ 495 lines (C), application ≈ 1 280 lines (C++), tests ≈ 275 lines.

## 4. How the Prototype Works – One Reading's Journey

```
1. SENSE   MonitorEngine thread → SensorDevice::readCelsius()
           → pread("/dev/tempsensor") → kernel → ts_read() → "60470"   (60.47 °C)
2. DECIDE  AlertManager::classify(60.47) → CRITICAL   (> 60 °C)
3. ACT     CoolingController::update(CRITICAL)
           → ioctl(TS_IOC_SET_COOLING, 1) → ts_ioctl() → CTRL.COOLING = 1
4. RECORD  TemperatureHistory::add()  and  LoggerProcess::log()
           → pipe → logger child process → tempmon.log
```

The thread then sleeps for one interval (a `condition_variable`, so it can be woken immediately on stop) and repeats. The menu runs in the main thread and stays responsive the whole time.

## 5. Build & Run

```bash
cd driver && make && sudo insmod tempsensor.ko     # load the driver
cd ../app && make                                   # build the application
./tempmon                                           # run
make test                                           # unit + integration tests (no driver needed)
```

## 6. Demonstration of the Initial Functionality

Environment: Ubuntu 26.04 LTS, Linux 7.0.0-30-generic, x86_64, Intel Core i5-12450H (2 cores assigned to the VM), sampling interval 200 ms.

### 6.1 Live monitoring and automatic cooling

Live view (`docs/screenshots/07_auto_cooling.png`):

```
[00:46:11]  59.57 C  WARNING   fan: OFF
[00:46:11]  60.25 C  CRITICAL  fan: ON      ← critical detected, fan switched on
[00:46:11]  57.58 C  WARNING   fan: ON
[00:46:12]  51.28 C  WARNING   fan: ON
[00:46:13]  46.12 C  WARNING   fan: ON      ← fan stays on (hysteresis)
[00:46:13]  44.82 C  NORMAL    fan: OFF     ← back to normal, fan switched off
```

Log file (`docs/screenshots/11_log_file.png`):

```
00:48:08 READING 60.47 C CRITICAL
00:48:08 ALERT WARNING -> CRITICAL at 60.47 C
00:48:08 FAN ON (automatic) - critical temperature
00:48:08 ALERT CRITICAL -> WARNING at 57.99 C
...
00:48:10 READING 44.51 C NORMAL
```

The device returned from Critical to Normal in about **2 seconds** after the fan was switched on.

### 6.2 Statistics after one session (`08_statistics.png`)

| Metric | Value |
|--------|-------|
| Readings | 392 |
| Minimum | 29.10 °C |
| Maximum | **60.92 °C** |
| Average | 45.19 °C |
| Normal / Warning / Critical | 164 / 223 / 5 |
| Automatic fan activations | 5 |

### 6.3 Effect of the protection

| Run | Maximum temperature |
|-----|---------------------|
| Driver alone, no protection (Stage 3, `04_proc_registers.png`) | **69.07 °C** (overheat flag set) |
| With the application's automatic cooling | **60.92 °C** |

Every time the temperature crossed 60 °C the fan was switched on at the next reading, so the device never stayed in the critical range.

### 6.4 Other functions shown

- System information (`09_system_info.png`): kernel 7.0.0-30-generic, x86_64 64-bit, 12th Gen Intel Core i5-12450H, 2 cores, 12 288 KB cache, 39-bit physical / 48-bit virtual addresses, little-endian, 4 096-byte pages, 3 398 MB memory
- Driver register status read through `ioctl` (menu option 9)
- CSV export (`readings.csv`)
- Safe shutdown with Ctrl+C (`10_ctrl_c_Shutting_down.png`): thread stopped, logger process finished, log saved

## 7. Development Progress, Issues, Design Challenges and Solutions

| # | Issue | Cause | Solution |
|---|-------|-------|----------|
| 1 | UEFI option not visible in VirtualBox settings | Settings shown in Basic mode | Switched to Expert mode and disabled UEFI (Secure Boot would block an unsigned module) |
| 2 | Red `vmwgfx` messages at first boot | Graphics driver warning under VirtualBox | Harmless; boot continued |
| 3 | VirtualBox crashed once after installation | VirtualBox application error on the host | Restarted the VM; Ubuntu booted normally |
| 4 | VM paused with `VERR_DISK_FULL` | Host C: drive full (VM disk grows dynamically) | Freed space on the host; planned moving the VM to another drive |
| 5 | Kernel API differences between kernel versions | `class_create()` and `devnode` signatures changed in newer kernels | `LINUX_VERSION_CODE` checks in the driver; builds unchanged on kernel 7.0 |
| 6 | `insmod: File exists` | Driver already loaded | Expected; unload with `rmmod` before reloading |
| 7 | Repeated `read()` calls on the same descriptor would return end-of-file | `read()` advances the file position | Application uses `pread(fd, buf, n, 0)` to always read from offset 0 |
| 8 | Ctrl+C had to work even while the menu waits for input | A blocking `getline` would not notice the signal | Signal handler only sets a flag; input waits with `poll()` and a timeout, so the flag is checked regularly |
| 9 | Stopping the monitor had to be immediate | A plain `sleep` would delay stopping by one interval | `condition_variable::wait_for` – `stop()` wakes the thread at once |
| 10 | `fork()` in a multithreaded program is unsafe | Child would copy a process with running threads | Logger process is started before any thread is created |
| 11 | Recovery time printed as "0 s" for short recoveries | Whole-second precision | Recovery time measured with 0.1 s precision |

## 8. Stage 4 Summary

### 8.1 Deliverables

| Deliverable | Location |
|-------------|----------|
| Working driver | `driver/` |
| C++ application (21 source files) | `app/` |
| Application tests | `tests/app_test.cpp` |
| Screenshots 06–12 | `docs/screenshots/` |
| This prototype log | `docs/04_Prototype_Log.md` |

### 8.2 Version control

- Branch `feature/app` created from `develop`
- Commits: application source, tests, screenshots
- `feature/app` merged into `develop`, then into `main`
- Tag: `stage-4`

### 8.3 Progress evidence

- Screenshots `06_app_monitoring.png` to `12_app_tests.png`
- Log file showing automatic fan activation and recovery
- `PROGRESS.md` entries for 4–5 October 2026

### 8.4 Demonstration

1. Load the driver, build and start `./tempmon`
2. Set the interval to 200 ms (option 8) and open the live view (option 3)
3. Show a CRITICAL reading, the fan switching on, the return to NORMAL and the fan switching off
4. Show statistics (4), driver registers (9), system information (10), CSV export (11)
5. Press Ctrl+C and show the safe shutdown, then `cat tempmon.log`

### 8.5 Roadmap to Stage 5

- Run the complete test plan: driver, unit, integration and system tests
- Record results, fix any defects found, review reliability and code quality
