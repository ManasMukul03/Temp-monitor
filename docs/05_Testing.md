# Stage 5 – Testing, Integration & Improvement

**Project:** Virtual Temperature Monitoring & Protection System
**Author:** Manas Mukul
**Version:** 1.0 – 5 October 2026

---

## 1. Test Strategy

Testing follows the layers of the system, from the smallest part to the complete system.

| Level | What is tested | How | Needs driver? |
|-------|----------------|-----|---------------|
| Driver tests | Every driver operation through system calls | `tests/driver_test.cpp` (automated) and shell commands (manual) | Yes |
| Unit tests | One application class at a time | `tests/app_test.cpp` with `MockSensor` | No |
| Integration tests | Classes working together: thread, logger process, cooling | `tests/app_test.cpp` | No |
| System tests | Complete application running against the real driver | Manual, with screenshots | Yes |

`MockSensor` implements the same `ITemperatureSource` interface as the real `SensorDevice`, so the application logic can be tested with exact, repeatable temperatures (for example a heating curve that crosses 60 °C and cools back below 45 °C).

**Test environment:** Ubuntu 26.04 LTS in VirtualBox, Linux 7.0.0-30-generic, x86_64, g++ with `-std=c++17 -Wall -Wextra`.

## 2. Driver Tests – Manual (shell)

| ID | Test | Expected | Result |
|----|------|----------|--------|
| M1 | Build the module with `make` | `tempsensor.ko` created, no errors | ✅ Pass (`01_driver_build.png`) |
| M2 | Load with `insmod`, check `dmesg` | "tempsensor: loaded, /dev/tempsensor major=… minor=0" | ✅ Pass (`02_driver_load.png`) |
| M3 | `lsmod` | Module listed | ✅ Pass |
| M4 | `ls -l /dev/tempsensor` | Character device `crw-rw-rw-` | ✅ Pass (`03_dev_file.png`) |
| M5 | `cat /dev/tempsensor` as normal user | Temperature in milli-°C | ✅ Pass (`03_dev_file_read.png`) |
| M6 | Read `/proc/tempsensor` several times | DATA changes, count increases | ✅ Pass |
| M7 | Wait for a high temperature | STATUS `overheat=1` above 60 °C | ✅ Pass – 69.07 °C (`04_proc_registers.png`) |
| M8 | `modinfo tempsensor.ko` | Author, licence, description | ✅ Pass |
| M9 | `rmmod`, then `ls /dev/tempsensor` | Device file removed | ✅ Pass |
| M10 | `dmesg` after unload | "tempsensor: unloaded", no errors | ✅ Pass |
| M11 | `insmod … interval_ms=500` | `INTERVAL : 500 ms` | ✅ Pass |
| M12 | `insmod … interval_ms=5` (invalid) | Falls back to `INTERVAL : 1000 ms` | ✅ Pass |
| M13 | Run `driver_test` | 11 passed | ✅ Pass |

## 3. Driver Tests – Automated (`tests/driver_test.cpp`)

| ID | Test | Result |
|----|------|--------|
| T1 | Open `/dev/tempsensor` | ✅ |
| T2 | Read returns a temperature within −40 … 125 °C | ✅ |
| T3 | `GET_INTERVAL` | ✅ |
| T4 | `SET_INTERVAL 200 ms` | ✅ |
| T5 | `SET_INTERVAL 5 ms` rejected with `EINVAL` | ✅ |
| T6 | `RESET` clears statistics | ✅ |
| T7 | Readings counted, min ≤ max | ✅ |
| T8 | STATUS shows ENABLED | ✅ |
| T9 | `SET_ENABLE 0` stops sampling | ✅ |
| T10 | Unknown ioctl rejected with `ENOTTY` | ✅ |
| T11 | Cooling fan brings temperature below 40 °C | ✅ |

**Result: 11 passed, 0 failed** (`05_driver_test.png`)

## 4. Unit Tests (`tests/app_test.cpp`)

| ID | Class | Test | Result |
|----|-------|------|--------|
| U1 | AlertManager | 30 °C → NORMAL | ✅ |
| U2 | AlertManager | 45 °C → WARNING (boundary) | ✅ |
| U3 | AlertManager | 60 °C → WARNING (boundary) | ✅ |
| U4 | AlertManager | 60.1 °C → CRITICAL | ✅ |
| U5 | AlertManager | Invalid thresholds (warning > critical) rejected | ✅ |
| U6 | AlertManager | New thresholds applied | ✅ |
| U7 | CoolingController | CRITICAL turns fan ON | ✅ |
| U8 | CoolingController | WARNING keeps fan ON (hysteresis) | ✅ |
| U9 | CoolingController | NORMAL turns fan OFF | ✅ |
| U10 | CoolingController | Manual mode ignores automatic rules | ✅ |
| U11 | TemperatureHistory | Minimum, maximum, average | ✅ |
| U12 | TemperatureHistory | Capacity limit removes the oldest reading | ✅ |
| U13 | TemperatureHistory | Counts per alert level | ✅ |
| U14 | ReportExporter | CSV header and rows written | ✅ |
| U15 | SystemInfo | Values read from `/proc` and `uname()` | ✅ |

## 5. Integration Tests (`tests/app_test.cpp`)

| ID | Components | Test | Result |
|----|------------|------|--------|
| I1 | LoggerProcess | Logger runs as a separate child process (`fork`) | ✅ |
| I2 | LoggerProcess | Messages travel through the pipe into the log file | ✅ |
| I3 | MonitorEngine | Monitoring thread starts | ✅ |
| I4 | MonitorEngine + MockSensor | Thread reads repeatedly and stops cleanly | ✅ |
| I5 | MonitorEngine + AlertManager + CoolingController | Heating curve 40 → 66 → 40 °C: fan ON exactly once, OFF after recovery | ✅ |
| I6 | MonitorEngine + LoggerProcess | Alerts and fan events written to the log | ✅ |

**Unit + integration result: 21 passed, 0 failed**, repeated runs stable (`12_app_tests.png`)

## 6. System Tests (application + real driver)

| ID | Scenario | Expected | Result |
|----|----------|----------|--------|
| S1 | Start `./tempmon` with the driver loaded | "Connected to /dev/tempsensor", monitoring starts automatically | ✅ Pass |
| S2 | Change interval to 200 ms (option 8) | Driver interval updated through ioctl | ✅ Pass |
| S3 | Live view (option 3) | New reading every interval, colour-coded levels | ✅ Pass (`06_app_monitoring.png`) |
| S4 | Overheating | At > 60 °C: CRITICAL, fan ON automatically | ✅ Pass – 60.25 °C → fan ON (`07_auto_cooling.png`) |
| S5 | Hysteresis | Fan stays ON through WARNING, OFF only at NORMAL | ✅ Pass – OFF at 44.82 °C |
| S6 | Statistics (option 4) | Min / max / average / counts consistent | ✅ Pass – 392 readings (`08_statistics.png`) |
| S7 | System information (option 10) | CPU, cache, memory, architecture shown | ✅ Pass (`09_system_info.png`) |
| S8 | Export CSV (option 11) | `readings.csv` created | ✅ Pass |
| S9 | Ctrl+C | Thread stopped, logger finished, log saved, clean exit | ✅ Pass (`10_ctrl_c_Shutting_down.png`) |
| S10 | Log file | Readings, alert changes, FAN ON / FAN OFF events | ✅ Pass (`11_log_file.png`) |
| S11 | Start `./tempmon` without the driver loaded | Clear error "cannot open /dev/tempsensor … Is the driver loaded?", exit code 1 | ✅ Pass |

## 7. Results

### 7.1 Overheating protection

| Run | Maximum temperature | Critical readings |
|-----|---------------------|-------------------|
| Driver alone, no protection | 69.07 °C | (device stayed overheated) |
| Application with automatic cooling (392 readings) | **60.92 °C** | 5 – each one immediately followed by fan ON |

- Recovery from Critical to Normal: about **2 seconds** (200 ms interval)
- Automatic fan activations: **5**, each followed by a recovery and fan OFF
- Distribution: 164 Normal, 223 Warning, 5 Critical

### 7.2 Test summary

| Category | Tests | Passed |
|----------|-------|--------|
| Driver – manual | 13 | 13 |
| Driver – automated | 11 | 11 |
| Application – unit | 15 | 15 |
| Application – integration | 6 | 6 |
| System | 11 | 11 |
| **Total** | **56** | **56** |

## 8. Debugging, Fixes and Improvements

### 8.1 Defects found and fixed

| Defect | Fix |
|--------|-----|
| Recovery time shown as "0 s" for short recoveries | Measured with 0.1 s precision |
| Shutdown message said "Ctrl+C received" also when input ended (Ctrl+D) | Separate flag for signals; correct message for each case |
| Generated files (`tempmon`, `app_test`, `*.log`, `*.csv`, `*.ko`) could be committed | Added to `.gitignore` |

### 8.2 Reliability

- **Clean shutdown order:** thread stopped → fan switched off → logger closed → child process reaped with `waitpid` (no zombie process)
- **Thread safety:** shared data protected by `std::mutex` (history, thresholds, fan state); spinlock in the driver between the timer and system calls
- **Safe signal handling:** the handler only sets a `sig_atomic_t` flag; all real work happens in normal code
- **RAII:** `SensorDevice` closes the device in its destructor, even if an exception occurs
- **Input validation:** driver rejects invalid intervals (`EINVAL`) and unknown commands (`ENOTTY`); application validates thresholds and numbers
- **Error handling:** a failed read stops the monitor with a clear error message instead of crashing

### 8.3 Performance

- Logging happens in a **separate process**, so disk writes never delay the monitoring thread
- The thread waits with a `condition_variable`, so it uses no CPU between readings and stops immediately
- Stable at a 200 ms interval for 392 readings with no errors

### 8.4 Code quality

- Builds with `-Wall -Wextra` and **zero warnings** (driver and application)
- One class per responsibility, header/source separation, comments on every key function
- Interface-based design (`ITemperatureSource`) makes the logic testable without hardware

## 9. Stage 5 Summary

### 9.1 Deliverables

| Deliverable | Location |
|-------------|----------|
| Test plan and results | `docs/05_Testing.md` |
| Automated driver tests | `tests/driver_test.cpp` |
| Automated application tests | `tests/app_test.cpp` |
| Test screenshots | `docs/screenshots/` |

### 9.2 Version control

- Commits for tests, fixes and this document
- Merged into `main`, tag `stage-5`

### 9.3 Progress evidence

- 56 of 56 tests passed
- Screenshots 01–12, log file, statistics

### 9.4 Demonstration

```bash
cd app && make test              # 21 unit + integration tests
cd ../tests && ./driver_test     # 11 driver tests
```

Then show the live run with automatic cooling and the log file.

### 9.5 Roadmap to Stage 6

- Final review of the complete system
- Final documentation, README and project report
- Prepare the GitHub-based demonstration for the evaluation
