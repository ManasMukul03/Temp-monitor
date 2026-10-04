# 🌡️ Virtual Temperature Monitoring & Protection System

**A Linux character device driver with a multithreaded C++ monitoring application**

Author: **Manas Mukul** · Wipro Training – Batch 3 · ITER, SOA University
Platform: Ubuntu 26.04 LTS · Linux kernel 7.0 · C (driver) · C++17 (application)

---

## 📌 Overview

Overheating is one of the main causes of hardware failure: it damages components, causes unexpected shutdowns and can be a safety risk in servers, industrial machines and vehicles. Devices need **continuous, automatic temperature monitoring that also takes corrective action**.

This project builds such a system the way real embedded systems do it:

- A **Linux kernel driver** exposes a temperature sensor and a cooling fan as the device `/dev/tempsensor`.
- A **C++ application** reads the sensor through system calls, raises alerts, **switches the fan on automatically** when the temperature becomes critical, and logs every event as proof.

The sensor and fan are **simulated inside the driver** (approved by the trainer). Because the design follows real hardware (registers, `read`, `ioctl`), a physical sensor can replace the simulation by changing one driver function.

**Core cycle:** sense → decide → act → record

## ✨ Features

| Feature | Status |
|---------|--------|
| Character device `/dev/tempsensor` created automatically on load | ✅ |
| Simulated sensor with CTRL / STATUS / DATA / INTERVAL registers | ✅ |
| Kernel timer producing a new reading every interval | ✅ |
| `ioctl` commands: interval, status, statistics, reset, enable, cooling fan | ✅ |
| Register dump at `/proc/tempsensor` | ✅ |
| Automated driver tests (11 tests) | ✅ |
| C++ monitoring app: live display, Normal / Warning / Critical alerts | ✅ |
| Automatic cooling control (fan on at > 60 °C, off below 45 °C) | ✅ |
| Logger process (`fork` + `pipe`), clean shutdown on Ctrl+C | ✅ |
| Statistics, history, system info, CSV report | ✅ |
| Automated application tests (21 unit + integration tests) | ✅ |

## 📊 Results

| | Value |
|---|---|
| Maximum temperature **without** protection | 69.07 °C |
| Maximum temperature **with** automatic cooling | **60.92 °C** |
| Recovery time Critical → Normal | ≈ 2 s |
| Tests passed | **56 / 56** |

Every time the temperature crossed 60 °C, the application switched the fan on at the next reading and the device returned to normal.

## 🏗️ Architecture

```
┌───────────────────── USER SPACE ─────────────────────┐
│  C++ Monitoring App                                   │
│   MonitorEngine (thread) → AlertManager               │
│                          → CoolingController          │
│                          → Logger process (pipe)      │
└──────────────────────────┬───────────────────────────┘
          system calls: open · read · ioctl · close
┌──────────────────────────▼──── KERNEL SPACE ─────────┐
│  /dev/tempsensor  (character device, major/minor)    │
│  tempsensor driver: file_operations                  │
│  Registers: CTRL · STATUS · DATA · INTERVAL          │
│  Kernel timer → simulated sensor + cooling fan       │
└──────────────────────────────────────────────────────┘
```

Full design with UML diagrams: [`docs/03_Design.md`](docs/03_Design.md)

## 🧠 Concepts Demonstrated

| Area | Concepts |
|------|----------|
| Linux | Shell, `make`, `insmod`, `rmmod`, `dmesg`, `lsmod`, `/dev`, `/proc` |
| Device drivers | Kernel module, character device, major/minor numbers, `file_operations`, kernel timer, `ioctl`, `copy_to_user`, spinlock |
| System programming | System calls, threads, mutex, signals, `fork`, `pipe` |
| C++ | Classes, interface + inheritance, STL, file handling, multi-file project |
| Computer architecture | Device registers, user mode vs kernel mode, CPU/memory information |
| Hardware & software | How a system call travels from an application to a device |

## 📁 Repository Structure

```
temp-monitor/
├── driver/
│   ├── tempsensor.c          # Kernel driver (C)
│   ├── tempsensor_ioctl.h    # Shared interface: register bits, ioctl commands
│   └── Makefile              # Builds the kernel module
├── app/                      # C++ monitoring application
│   ├── main.cpp              # Menu, live view, Ctrl+C handling
│   ├── MonitorEngine.*       # Background thread: sense → decide → act → record
│   ├── SensorDevice.*        # System calls on /dev/tempsensor
│   ├── AlertManager.*        # Normal / Warning / Critical
│   ├── CoolingController.*   # Automatic fan control
│   ├── TemperatureHistory.*  # History and statistics
│   ├── LoggerProcess.*       # Logger child process (fork + pipe)
│   ├── SystemInfo.*          # CPU / memory / architecture info
│   ├── ReportExporter.*      # CSV export
│   ├── ITemperatureSource.h  # Sensor interface
│   ├── MockSensor.h          # Fake sensor for tests
│   └── Makefile
├── tests/
│   ├── driver_test.cpp       # Automated driver tests
│   └── app_test.cpp          # Application unit + integration tests
├── docs/
│   ├── 01_Introduction.md    # Stage 1 – Project introduction
│   ├── 02_PRD.md             # Stage 2 – Requirements & development plan
│   ├── 03_Design.md          # Stage 3 – Architecture, UML, environment setup
│   ├── 04_Prototype_Log.md   # Stage 4 – Implementation & prototype
│   ├── 05_Testing.md         # Stage 5 – Testing & improvement
│   ├── 06_Final_Summary.md   # Stage 6 – Final summary
│   └── screenshots/          # Progress evidence
├── PROGRESS.md               # Daily progress log
└── README.md
```

## ⚙️ Requirements

- Ubuntu 26.04 LTS (or any Linux with kernel headers), tested on kernel 7.0.0-30-generic
- Secure Boot disabled (self-built modules are unsigned)
- Packages:

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) git
```

## 🚀 Build & Run

### 1. Get the code

```bash
git clone https://github.com/ManasMukul03/Temp-monitor.git
cd Temp-monitor
```

### 2. Build and load the driver

```bash
cd driver
make                          # builds tempsensor.ko
sudo insmod tempsensor.ko     # loads the driver, creates /dev/tempsensor
sudo dmesg | tail -3          # "tempsensor: loaded, /dev/tempsensor major=... minor=0"
```

Optional: choose the sampling interval when loading:

```bash
sudo insmod tempsensor.ko interval_ms=500
```

### 3. Check the device

```bash
ls -l /dev/tempsensor         # crw-rw-rw- ... character device
cat /dev/tempsensor           # current temperature in milli-°C, e.g. 36250 = 36.25 °C
cat /proc/tempsensor          # register dump
lsmod | grep tempsensor       # driver is loaded
```

Example `/proc/tempsensor` output:

```
Virtual Temperature Sensor - register dump
CTRL     : 0x00000001  (enable=1 cooling=0)
STATUS   : 0x00000007  (data_ready=1 overheat=1 enabled=1 cooling=0)
DATA     : 69068 m°C  (69.068 °C)
INTERVAL : 1000 ms
STATS    : min=35610 max=69138 count=38
DEVICE   : major=239 minor=0 open_count=0
```

### 4. Run the driver tests

```bash
cd ../tests
g++ -std=c++17 -Wall -I../driver driver_test.cpp -o driver_test
./driver_test                 # Result: 11 passed, 0 failed
```

### 5. Build and run the monitoring application

```bash
cd ../app
make                          # builds ./tempmon
./tempmon                     # driver must be loaded
```

Menu options:

```
 1. Start monitoring          7. Cooling fan control (auto / manual)
 2. Stop monitoring           8. Sampling interval
 3. Live view                 9. Driver status (registers)
 4. Statistics               10. System information
 5. Recent readings          11. Export CSV report
 6. Alert thresholds          0. Exit
```

Demo tip: set the interval to **200 ms** (option 8), then open the **live view** (option 3) to watch a CRITICAL reading switch the fan on and the temperature return to NORMAL. Press **Ctrl+C** at any time for a safe shutdown; events are saved in `tempmon.log`.

### 6. Run the application tests

```bash
make test                     # Result: 21 passed, 0 failed (no driver needed)
```

### 7. Unload the driver

```bash
sudo rmmod tempsensor
```

> After changing the driver source: `sudo rmmod tempsensor` → `make` → `sudo insmod tempsensor.ko`.
> After a reboot the driver must be loaded again with `insmod`.

## 🔌 Driver Interface

| Call | Description |
|------|-------------|
| `read()` | Latest temperature as text in milli-°C |
| `ioctl(TS_IOC_SET_INTERVAL, &u32)` | Sampling interval, 100–10000 ms (else `EINVAL`) |
| `ioctl(TS_IOC_GET_INTERVAL, &u32)` | Current interval |
| `ioctl(TS_IOC_GET_STATUS, &u32)` | STATUS register |
| `ioctl(TS_IOC_GET_STATS, &stats)` | Minimum, maximum, reading count |
| `ioctl(TS_IOC_RESET)` | Clear statistics, fan off |
| `ioctl(TS_IOC_SET_ENABLE, &u32)` | Sampling on (1) / off (0) |
| `ioctl(TS_IOC_SET_COOLING, &u32)` | Cooling fan on (1) / off (0) |

**Register map**

| Register | Bits |
|----------|------|
| CTRL | bit 0 ENABLE · bit 1 COOLING |
| STATUS | bit 0 DATA_READY · bit 1 OVERHEAT (> 60 °C) · bit 2 ENABLED · bit 3 COOLING |
| DATA | temperature, signed milli-°C |
| INTERVAL | sampling period, ms |

## 🧪 Testing

| Category | Tests | Passed |
|----------|-------|--------|
| Driver – manual | 13 | 13 |
| Driver – automated (`driver_test`) | 11 | 11 |
| Application – unit (`app_test`) | 15 | 15 |
| Application – integration (`app_test`) | 6 | 6 |
| System (app + driver) | 11 | 11 |

Full results: [`docs/05_Testing.md`](docs/05_Testing.md)

**Driver tests**

| Test | Checks |
|------|--------|
| T1–T2 | Device opens; reading is within sensor limits |
| T3–T5 | Interval get/set; invalid interval rejected with `EINVAL` |
| T6–T7 | Reset clears statistics; readings are counted, min ≤ max |
| T8–T9 | Status register; disabling stops sampling |
| T10 | Unknown `ioctl` rejected with `ENOTTY` |
| T11 | Cooling fan brings the temperature below 40 °C |

Current result: **11 passed, 0 failed** on kernel 7.0.0-30-generic.

## 📚 Documentation (6 stages)

| Stage | Document | Status |
|-------|----------|--------|
| 1 – Project Introduction | [`docs/01_Introduction.md`](docs/01_Introduction.md) | ✅ |
| 2 – Requirements & Development Plan | [`docs/02_PRD.md`](docs/02_PRD.md) | ✅ |
| 3 – System Design & Architecture | [`docs/03_Design.md`](docs/03_Design.md) | ✅ |
| 4 – Initial Implementation & Prototype | [`docs/04_Prototype_Log.md`](docs/04_Prototype_Log.md) | ✅ |
| 5 – Testing, Integration & Improvement | [`docs/05_Testing.md`](docs/05_Testing.md) | ✅ |
| 6 – Final Implementation & Presentation | [`docs/06_Final_Summary.md`](docs/06_Final_Summary.md) | ✅ |

Daily progress: [`PROGRESS.md`](PROGRESS.md)

## 🌿 Git Workflow

- `main` – stable code, tagged at the end of each stage (`stage-1`, `stage-2`, `stage-3`, …)
- `develop` – integration branch
- `feature/*` – one branch per module (`feature/driver`, `feature/app`)

## 🔭 Future Improvements

- Replace the simulated sensor with a real one (e.g. DS18B20 on a Raspberry Pi) by changing only `sensor_read_hw()` in the driver
- Control a real fan through a GPIO pin
- Blocking reads / `poll()` support so the application wakes up only when new data is ready
- Several sensors (`/dev/tempsensor0`, `/dev/tempsensor1`, …) and PWM fan speed
- Remote monitoring over the network

## ⚠️ Limitations

- Sensor and fan are simulated in the driver (approved by the trainer)
- The application polls at a fixed interval
- Console interface only
