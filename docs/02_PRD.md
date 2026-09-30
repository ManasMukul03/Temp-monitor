# Stage 2 – Project Requirements Document (PRD) & Development Plan

**Project:** Virtual Temperature Monitoring System
**Author:** Manas Mukul
**Version:** 1.0 – 29 September 2026

---

## 1. Overview

The system consists of a Linux kernel driver that simulates a temperature sensor and a C++ application that monitors it. This document lists what the system must do (functional requirements), how well it must do it (non-functional requirements), its modules, deliverables and the development timeline.

## 2. Users

| User | Needs |
|------|-------|
| Operator | Start monitoring, see live temperature, receive alerts, view statistics and reports |
| Developer / Evaluator | Load and unload the driver, inspect kernel logs, run tests, read documentation |

## 3. Modules

| ID | Module | Responsibility |
|----|--------|----------------|
| M1 | Temperature Sensor Driver (kernel) | Creates `/dev/tempsensor`, simulates the sensor and its registers, answers `read` and `ioctl` |
| M2 | Sensor Device Interface (C++) | Wraps `open`, `read`, `ioctl`, `close` in a C++ class |
| M3 | Monitoring Engine | Background thread that polls the sensor at the chosen interval |
| M4 | Alert Manager | Classifies readings as Normal / Warning / Critical |
| M5 | Statistics & History | Stores recent readings, calculates minimum, maximum and average |
| M6 | Logger Process | Separate process that receives messages through a pipe and writes the log file |
| M7 | System Information | Reads CPU and memory details from `/proc/cpuinfo` and `/proc/meminfo` |
| M8 | Report Exporter | Saves readings to a CSV file |
| M9 | Console Menu | User interface connecting all modules |
| M10 | Cooling Controller | Switches the cooling fan on at Critical and off at Normal (automatic protection) |

## 4. Functional Requirements

| ID | Requirement | Module |
|----|-------------|--------|
| FR1 | The driver shall register a character device and create `/dev/tempsensor` when loaded | M1 |
| FR2 | The driver shall generate a new simulated reading at a fixed interval using a kernel timer | M1 |
| FR3 | Reading `/dev/tempsensor` shall return the latest temperature | M1 |
| FR4 | The driver shall support `ioctl` commands to set the sampling interval, read the status register, read min/max statistics and reset the sensor | M1 |
| FR5 | The driver shall remove the device and free all resources when unloaded | M1 |
| FR6 | The application shall open the device and report a clear error if the driver is not loaded | M2 |
| FR7 | The application shall display the live temperature, updated automatically | M3, M9 |
| FR8 | The application shall classify each reading as Normal (< 45 °C), Warning (45–60 °C) or Critical (> 60 °C) | M4 |
| FR9 | The user shall be able to change the alert thresholds | M4 |
| FR10 | The application shall show minimum, maximum and average temperature | M5 |
| FR11 | The application shall keep a history of the most recent readings | M5 |
| FR12 | All readings and alerts shall be written to a log file by a separate logger process | M6 |
| FR13 | The user shall be able to change the sampling interval | M1, M3 |
| FR14 | The application shall display CPU and memory information of the system | M7 |
| FR15 | The user shall be able to export readings to a CSV report | M8 |
| FR16 | Pressing Ctrl+C shall stop monitoring, save data and exit cleanly | M3, M6 |
| FR17 | The driver shall provide a simulated cooling fan controlled through `ioctl`; when it is on, the temperature shall fall | M1 |
| FR18 | The application shall switch the fan on automatically when a reading is Critical and off when it returns to Normal, and log both events | M10, M6 |
| FR19 | The user shall be able to turn automatic cooling on or off, or control the fan manually | M10, M9 |

## 5. Non-Functional Requirements

| Category | Requirement |
|----------|-------------|
| Performance | A sensor read shall complete in under 10 ms; polling interval adjustable from 500 ms to 10 s |
| Reliability | No crash on invalid input; clean shutdown on Ctrl+C; driver unloads without kernel errors |
| Resource safety | No memory leaks in driver or application; all file descriptors closed |
| Concurrency | Shared data protected by a mutex in the application and a spinlock/mutex in the driver |
| Usability | Clear menu, readable output, alerts clearly marked |
| Maintainability | Modular code split into header and source files, comments on key functions |
| Portability | Runs on Ubuntu 26.04 LTS (x86_64); sensor logic isolated so a real sensor can replace it |
| Version control | Work committed to Git at least once per development day |

## 6. Driver Interface (summary)

| Operation | Behaviour |
|-----------|-----------|
| `open` / `close` | Start / end access to the sensor |
| `read` | Returns the latest temperature as text in milli-degrees Celsius (e.g. `36250` = 36.25 °C) |
| `ioctl SET_INTERVAL` | Sets how often the sensor produces a new reading |
| `ioctl GET_STATUS` | Returns the status register (data ready, overheat flag) |
| `ioctl GET_STATS` | Returns minimum, maximum and number of readings |
| `ioctl RESET` | Clears statistics and restarts the sensor |
| `ioctl SET_COOLING` | Turns the simulated cooling fan on or off |

Full details will be defined in the Stage 3 design document.

## 7. Development Environment

| Item | Choice |
|------|--------|
| OS | Ubuntu 26.04 LTS in a VirtualBox virtual machine |
| Compilers | gcc (driver, C), g++ with C++17 (application) |
| Build | GNU make, Linux kernel headers |
| Tools | `insmod`, `rmmod`, `lsmod`, `dmesg` |
| Version control | Git + GitHub |
| Editor | VS Code |
| Diagrams | draw.io / Mermaid |

## 8. Deliverables

1. Driver source code (`driver/`)
2. C++ application source code (`app/`)
3. Test programs and test report (`tests/`, `docs/05_Testing.md`)
4. Six stage documents (`docs/`)
5. UML diagrams: class, sequence, state machine (`docs/diagrams/`)
6. Progress log (`PROGRESS.md`) and Git history
7. Final project report (PDF) and presentation

## 9. Development Plan & Timeline

| Date | Stage | Milestone |
|------|-------|-----------|
| 29 Sep | 1 & 2 | Introduction, PRD, environment setup, Git repository |
| 30 Sep | 3 | Architecture, UML diagrams, driver prototype |
| 01 Oct | 4 | C++ application integrated with driver (working prototype) |
| 02 Oct | 5 | Unit, integration and system testing; bug fixes |
| 03 Oct | 6 | Final report, presentation, final Git release |
| 04 Oct | – | Buffer day |
| 05 Oct | – | Submission deadline |

## 10. Risks & Mitigation

| Risk | Mitigation |
|------|-----------|
| Kernel module fails to load (headers mismatch, Secure Boot) | Install headers for the running kernel; keep Secure Boot disabled in the VM |
| Driver bug crashes the VM | Test in the VM only; take a VirtualBox snapshot before loading the driver |
| Race conditions between threads | Protect shared data with a mutex; test with fast polling |
| Tight schedule | Build the core first (driver + read), add features after the prototype works |

## 11. Acceptance Criteria

- Driver loads, creates `/dev/tempsensor`, and unloads cleanly with no errors in `dmesg`
- Application shows live readings and correct alert levels
- When the temperature becomes Critical, the fan turns on automatically and the temperature returns to Normal
- Log file and CSV report are created with correct data
- Ctrl+C exits cleanly with data saved
- All planned test cases pass
- Documentation and Git history are complete

---

## 12. Stage 2 Summary

### 12.1 Deliverables

| Deliverable | Location |
|-------------|----------|
| Project Requirements Document | `docs/02_PRD.md` |
| 10 modules (M1–M10), 19 functional requirements, non-functional requirements | Sections 3–5 |
| Driver interface summary | Section 6 |
| Development plan, timeline, risks, acceptance criteria | Sections 9–11 |

### 12.2 Version control

- Commit: `docs: stage 2 requirements and development plan`
- Tag: `stage-2`

### 12.3 Progress evidence

- `PROGRESS.md` entry for 29 Sep 2026
- Every requirement has an ID (FR1–FR19) so it can be traced to design (Stage 3) and tests (Stage 5)

### 12.4 Demonstration

Stage 2 is presented by walking through:

1. The modules table: how the system is divided and which module owns which job
2. Key functional requirements: FR8 (alert levels), FR12 (logger process), FR16 (Ctrl+C), FR17–FR18 (automatic cooling)
3. Non-functional requirements: performance, reliability, concurrency, portability
4. The timeline and the risk table, showing how problems such as a driver crash are prevented

### 12.5 Roadmap to Stage 3

- Design the architecture across user space and kernel space
- Define the register map, the ioctl interface and the data structures
- Draw class, sequence and state machine diagrams
- Set up the Ubuntu development environment and the Git branching strategy
