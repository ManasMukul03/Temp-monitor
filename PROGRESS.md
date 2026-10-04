# Progress Log

## 29 Sep 2026 — Stage 1 & 2
**Done**
- Chose project: Virtual Temperature Monitoring System (simulated sensor approved by trainer)
- Wrote Stage 1 Introduction (`docs/01_Introduction.md`)
- Wrote Stage 2 PRD and development plan (`docs/02_PRD.md`)
- Planned environment: Ubuntu 26.04 VM in VirtualBox

**Issues & solutions**
- None

## 30 Sep 2026 — Stage 3 + driver prototype
**Done**
- Set up Ubuntu 26.04 VM (kernel 7.0), installed build tools and kernel headers
- Created project structure and Git repository
- Stage 3 design document (`docs/03_Design.md`): architecture, components, interfaces, data structures, register map
- UML: class diagram, 4 sequence diagrams, 4 state machine diagrams
- Git branching strategy: main / develop / feature branches, tag per stage
- Driver `driver/tempsensor.c`: character device, simulated registers, kernel timer, read, ioctl, /proc/tempsensor
- Shared interface header `driver/tempsensor_ioctl.h`
- Added automatic overheating protection: simulated cooling fan in the driver (CTRL bit 1, ioctl SET_COOLING)
- Driver test program `tests/driver_test.cpp` (11 tests)

**Issues & solutions**
- UEFI option hidden in VirtualBox Basic settings: switched to Expert mode and disabled UEFI
- vmwgfx graphics warning at first boot: harmless, boot continued
- VirtualBox crashed once after installation: restarted the VM, Ubuntu booted normally
- Driver built on kernel 7.0.0-30 without code changes; "module verification failed" taint message is expected for unsigned self-built modules
- All 11 driver tests passed; overheat flag observed at 69 °C, cooling fan test brought temperature below 40 °C

**Next (1 Oct)**
- C++ application core: SensorDevice, AlertManager, TemperatureHistory
- MonitorEngine thread

## 4 Oct 2026 — Stage 4 prototype
**Done**
- Repeated all driver tests manually (M1–M13) and saved screenshots 01–05
- C++ application in `app/`: SensorDevice (system calls), AlertManager, CoolingController, TemperatureHistory, MonitorEngine thread, LoggerProcess (fork + pipe), SystemInfo, ReportExporter, console menu with Ctrl+C handling
- Application tests (`tests/app_test.cpp`): 21 passed
- Branch `feature/app` created and merged into `develop`

**Issues & solutions**
- `insmod: File exists`: driver was already loaded; continued without reloading
- Host C: drive filled up and VirtualBox paused the VM: freed space on the host and resumed

**Next (5 Oct)**
- System tests with the real driver, Stage 4–6 documents, final README

## 5 Oct 2026 — Stage 5 & 6
**Done**
- System tests with the driver: automatic cooling observed live (60.25 °C → fan ON → 44.82 °C → fan OFF)
- Session statistics: 392 readings, max 60.92 °C (69.07 °C without protection), 5 automatic fan activations, recovery ≈ 2 s
- Screenshots 06–12
- Documents: `04_Prototype_Log.md`, `05_Testing.md`, `06_Final_Summary.md`; README updated
- All 56 tests passed

**Next**
- Project evaluation with the trainer
