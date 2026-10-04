# Stage 6 – Final Implementation & Presentation

**Project:** Virtual Temperature Monitoring & Protection System
**Author:** Manas Mukul
**Version:** 1.0 – 5 October 2026

---

## 1. Final System

A complete temperature monitoring and protection system built on Linux:

- A **Linux kernel driver** (C) exposes a simulated temperature sensor and cooling fan as the character device `/dev/tempsensor`, with hardware-style registers, a kernel timer, `read`, `ioctl` and a `/proc` register dump.
- A **multithreaded C++ application** monitors the device through system calls, classifies every reading, switches the fan automatically when the temperature becomes critical, keeps statistics, logs every event through a separate logger process and shuts down safely on Ctrl+C.

**Problem solved:** overheating damages hardware and causes downtime. The system detects overheating and corrects it automatically, and the logs prove it: the maximum temperature fell from **69.07 °C** without protection to **60.92 °C** with protection, and each critical reading returned to normal in about **2 seconds**.

## 2. Final Architecture

```
┌──────────────────────────── USER SPACE ─────────────────────────────┐
│  tempmon (C++)                                                       │
│   main thread: ConsoleMenu ── SIGINT handler (Ctrl+C)               │
│   monitor thread: MonitorEngine                                      │
│        sense  → SensorDevice ── open / pread / ioctl ───────────┐   │
│        decide → AlertManager                                      │   │
│        act    → CoolingController ── ioctl(SET_COOLING) ─────────┤   │
│        record → TemperatureHistory, LoggerProcess ──pipe──► child │   │
│                                                    process → log  │   │
│   SystemInfo ◄── /proc/cpuinfo, /proc/meminfo, uname()            │   │
│   ReportExporter → readings.csv                                   │   │
└───────────────────────────────────────────────────────────────────┼───┘
                                    system calls                     │
┌───────────────────────────────── KERNEL SPACE ─────────────────────▼───┐
│  /dev/tempsensor (char device, major/minor) → file_operations          │
│  ts_open · ts_read · ts_ioctl · ts_release        /proc/tempsensor     │
│  Registers: CTRL (enable, cooling) · STATUS · DATA · INTERVAL          │
│  Kernel timer → sensor_read_hw() (simulated sensor + fan), spinlock    │
└────────────────────────────────────────────────────────────────────────┘
```

Detailed design and UML diagrams: `docs/03_Design.md`.

## 3. Implementation Summary

| Part | Language | Files | Lines |
|------|----------|-------|-------|
| Kernel driver + shared header | C | 2 | ≈ 495 |
| Monitoring application | C++17 | 21 | ≈ 1 280 |
| Tests (driver + application) | C++17 | 2 | ≈ 275 |
| **Total** | | **25** | **≈ 2 050** |

### Concepts demonstrated

| Training topic | Where in the project |
|----------------|----------------------|
| Linux | Ubuntu 26.04, kernel 7.0, shell tools, `make`, `insmod`, `rmmod`, `dmesg`, `/dev`, `/proc` |
| Device drivers | Loadable kernel module, character device, major/minor numbers, `file_operations`, `copy_to_user` / `copy_from_user`, kernel timer, spinlock, `ioctl`, `/proc` entry, module parameter |
| System programming | `open`, `pread`, `ioctl`, `close`, `fork`, `pipe`, `waitpid`, `poll`, signals (`sigaction`), threads, mutexes, condition variable |
| C++ | Classes, abstract interface and inheritance, polymorphism, RAII, STL (`deque`, `vector`, `map`, `optional`), file streams, multi-file project with Makefile |
| Computer architecture | Device registers and bit fields, user mode vs kernel mode, CPU / cache / address sizes / byte order / page size |
| Hardware & software | Path of a request from an application through a system call to a device; hardware-independent design |
| Software process | Six documented stages, PRD, UML, Git branches and tags, unit / integration / system testing |

## 4. Testing and Results

| Category | Passed |
|----------|--------|
| Driver – manual | 13 / 13 |
| Driver – automated | 11 / 11 |
| Application – unit | 15 / 15 |
| Application – integration | 6 / 6 |
| System | 11 / 11 |
| **Total** | **56 / 56** |

| Result | Value |
|--------|-------|
| Maximum temperature without protection | 69.07 °C |
| Maximum temperature with automatic cooling | 60.92 °C |
| Recovery time Critical → Normal | ≈ 2 s |
| Readings in test session | 392 (no errors) |
| Automatic fan activations | 5 |

Full test details: `docs/05_Testing.md`.

## 5. Demonstration Plan (5–10 minutes)

| Time | Show | Say |
|------|------|-----|
| 0:00–1:00 | GitHub README | The problem (overheating), the solution, the sense → decide → act → record cycle |
| 1:00–2:30 | `docs/03_Design.md` diagrams on GitHub | User space vs kernel space, driver registers, the protection sequence diagram |
| 2:30–4:00 | Terminal: `make`, `insmod`, `dmesg`, `ls -l /dev/tempsensor`, `cat /proc/tempsensor` | The driver creates the device; registers; overheat bit |
| 4:00–7:00 | `./tempmon`, interval 200 ms, live view | Critical → fan ON automatically → Normal → fan OFF; statistics; system information |
| 7:00–8:00 | Ctrl+C, `cat tempmon.log` | Safe shutdown, logger process, proof in the log |
| 8:00–9:00 | `make test`, `./driver_test` | 56 tests passing, Git branches and tags |

## 6. Achievements

- A real Linux character device driver that loads on kernel 7.0 and creates its device automatically
- A complete sense → decide → act → record control loop that keeps the device out of the critical range
- Hardware-style register interface (CTRL / STATUS / DATA / INTERVAL) with bit fields
- Multithreaded application with a separate logger process and safe signal handling
- Interface-based design that allows testing without hardware and replacing the sensor easily
- 56 of 56 tests passing; zero compiler warnings
- Professional process: six stage documents, PRD, UML, Git feature branches, stage tags, progress log

## 7. Limitations

- The sensor and fan are **simulated**; no physical hardware is used (approved by the trainer)
- The application **polls** the driver at a fixed interval instead of being notified when new data is ready
- One sensor and one fan only
- Console interface only; no graphical or remote interface
- Logs and history are kept on the local machine only

## 8. Future Improvements

| Improvement | Change required |
|-------------|-----------------|
| Real sensor (e.g. DS18B20 on a Raspberry Pi) | Replace only `sensor_read_hw()` in the driver; the application stays the same |
| Real fan | Drive a GPIO pin from the cooling bit in the driver |
| Event-driven reading | Implement `poll()` / wait queue in the driver so the app wakes only when data is ready |
| Several sensors | Multiple minor numbers (`/dev/tempsensor0`, `/dev/tempsensor1`, …) |
| Proportional fan speed | PWM instead of on/off control |
| Remote monitoring | Send readings over the network to a dashboard |

## 9. Final Deliverables

| Deliverable | Location |
|-------------|----------|
| Source code – driver | `driver/` |
| Source code – application | `app/` |
| Tests | `tests/` |
| Stage documents 1–6 | `docs/01_Introduction.md` … `docs/06_Final_Summary.md` |
| UML diagrams | `docs/03_Design.md` (class, sequence, state machine) |
| Screenshots | `docs/screenshots/` |
| Progress log | `PROGRESS.md` |
| Build and run instructions | `README.md` |
| Git repository | github.com/ManasMukul03/Temp-monitor (branches `main`, `develop`, `feature/*`; tags `stage-1` … `stage-6`) |

## 10. Stage 6 Summary

### 10.1 Version control

- Final documentation merged into `main`
- Tag: `stage-6`

### 10.2 Progress evidence

- All six stage documents, 12 screenshots, test results, Git history with tags

### 10.3 Demonstration

See the demonstration plan in Section 5.

### 10.4 Next steps

- Project evaluation with the trainer
- Possible extension to real hardware as described in Section 8
