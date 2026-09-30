# Stage 1 – Project Introduction

**Project Title:** Virtual Temperature Monitoring System
**Subtitle:** A Linux Character Device Driver with a Multithreaded C++ Monitoring Application
**Author:** Manas Mukul
**Program:** Wipro Training – Batch 3 (ITER, SOA University)
**Date:** 29 September 2026

---

## 1. Project Idea

Real embedded systems (industrial machines, servers, cars, IoT devices) use temperature sensors to detect overheating. The operating system talks to such a sensor through a **device driver**, and applications read the sensor through **system calls**.

This project builds that complete path in software:

1. A **Linux kernel module** that behaves like a temperature sensor and appears as the device file `/dev/tempsensor`.
2. A **C++ monitoring application** that reads the sensor through system calls, watches the temperature in real time, raises alerts, keeps statistics and writes logs.
3. **Automatic overheating protection:** when the temperature becomes critical, the application switches on a cooling fan through the driver, and switches it off again once the device is safe.

No physical hardware is required: the sensor is **simulated inside the driver**. This approach was confirmed with the trainer.

## 2. Objective

To design and develop a Linux-based temperature monitoring system that demonstrates:

- Writing and loading a **Linux character device driver**
- Communicating between user space and kernel space using **system calls** (`open`, `read`, `ioctl`, `close`)
- **System programming** concepts: threads, synchronization, signals, processes and inter-process communication (pipes)
- **C++ programming** with OOP, STL and file handling
- **Computer architecture** concepts: device registers, user/kernel mode, CPU and memory information
- A professional **software development process**: requirements, design, implementation, testing and documentation with Git version control

## 3. Problem Statement

### 3.1 The real-world problem

Heat is one of the main causes of failure in electronic hardware. When a server, an industrial machine or a vehicle engine overheats and the problem is not noticed in time:

- The hardware can be damaged, or its lifetime shortened
- Systems shut down unexpectedly, causing downtime and loss of work
- In industrial and automotive settings, overheating can become a safety risk

Manual checking is too slow and unreliable. Devices therefore need **continuous, automatic temperature monitoring** that reacts faster than a human can, and that **takes corrective action by itself**, the way a server speeds up its fans or a car switches on its radiator fan.

### 3.2 What a monitoring system must do

| Need | How this project solves it |
|------|---------------------------|
| Read the sensor continuously without freezing the software | A background thread polls the sensor while the menu stays responsive |
| Detect danger immediately | Every reading is classified as Normal, Warning or Critical and alerts are raised at once |
| Respond automatically, without waiting for a human | At Critical level the application turns on the cooling fan through the driver (`ioctl`); it turns the fan off when the temperature is back to Normal |
| Keep evidence for later analysis | A separate logger process records every reading and alert in a log file; readings can be exported as a CSV report |
| Never lose data when the program is stopped | A signal handler (Ctrl+C) stops monitoring and saves all data before exiting |
| Support different sensors | All sensor logic is isolated inside the driver, so a real sensor can replace the simulated one without changing the application |

### 3.3 Why this approach matters

A simple program could print random numbers and call itself a monitor. This project instead follows the **architecture used by real systems**: the sensor exists behind a device driver in the Linux kernel, and the application reaches it only through system calls (`open`, `read`, `ioctl`). This is how real devices work, from a laptop's CPU temperature sensor to an engine monitor in a car.

Because of this design, the project demonstrates:

- **Correctness of architecture:** the same user-space/kernel-space separation as production systems
- **Reliability:** non-blocking monitoring, immediate alerts, safe shutdown and persistent logs
- **Closed-loop control:** the system does not only report a problem, it corrects it (sense → decide → act), and the logs prove how quickly it did
- **Extensibility:** moving to real hardware requires changing only the function that produces a reading inside the driver

### 3.4 Problem summary

> Overheating damages hardware and causes downtime. This project builds a complete temperature monitoring and protection system in the same way real embedded systems do it: a Linux driver exposes the sensor and a cooling fan as a device, and a multithreaded C++ application monitors it through system calls, raises alerts, automatically switches the fan on at critical temperature, logs data and shuts down safely.

## 4. Project Scope

**In scope**

| Area | Included |
|------|----------|
| Driver | Character device `/dev/tempsensor`, simulated sensor and cooling fan with registers, kernel timer, `read` and `ioctl` operations |
| System programming | Polling thread, mutex, signal handling (Ctrl+C), logger process using `fork` and `pipe` |
| Application | Live display, 3-level alerts, automatic cooling control, statistics, history, sampling-rate control, system information, CSV report |
| Documentation | Six stage documents, UML diagrams, test report, project report, Git history |

**Out of scope**

- A real physical sensor (listed as future work)
- Graphical user interface (the application is console-based)
- Network / cloud upload of readings

## 5. Training Topics Covered

| Topic | How the project uses it |
|-------|------------------------|
| Linux | Ubuntu, shell commands, `make`, `insmod`, `rmmod`, `dmesg`, `/dev`, `/proc` |
| Device drivers | Kernel module, character device, file operations, kernel timer, `ioctl`, `copy_to_user` |
| System programming | System calls, threads, mutex, signals, `fork`, `pipe` |
| C++ | Classes, inheritance, STL containers, file handling, multi-file project |
| Computer architecture | Simulated control/status/data registers, CPU and memory details from `/proc` |
| Hardware & software | User space vs kernel space, how a system call reaches a driver |

## 6. Expected Outcome

- A loadable kernel module that creates `/dev/tempsensor` and produces realistic temperature readings
- A C++ application that monitors the sensor in real time, alerts at warning and critical levels, automatically brings an overheating device back to a safe temperature, and stores logs and reports
- Complete documentation, UML diagrams, test results and a Git repository showing step-by-step progress

## 7. Applications

- **Server rooms / data centers:** overheating alarms
- **Industrial machines:** motor and furnace temperature monitoring
- **Automobiles:** engine temperature monitoring
- **Smart homes & IoT:** room temperature control
- **Education:** a clear example of the full driver → system call → application stack

The same design works with a real sensor: only the function that produces the reading inside the driver needs to change.

---

## 8. Stage 1 Summary

### 8.1 Deliverables

| Deliverable | Location |
|-------------|----------|
| Project introduction (idea, objective, problem, scope, outcome, applications) | `docs/01_Introduction.md` |
| Repository structure and README | `README.md` |
| Progress log started | `PROGRESS.md` |

### 8.2 Version control

- Git repository initialised on branch `main`
- Commit: `docs: stage 1 project introduction`
- Tag: `stage-1`

### 8.3 Progress evidence

- `PROGRESS.md` entry for 29 Sep 2026
- Trainer confirmation that a simulated temperature sensor implemented as a Linux kernel module is acceptable

### 8.4 Demonstration

Stage 1 is presented as a short talk:

1. **The problem:** overheating damages hardware, causes downtime and can be a safety risk
2. **The solution:** a Linux driver exposes a temperature sensor and a cooling fan; a C++ application monitors the temperature and protects the device automatically
3. **The scope:** what is included (driver, system programming, C++ application) and what is not (physical sensor, GUI, network)
4. **The outcome:** a working detect → alert → cool-down system with logs as proof

### 8.5 Roadmap to Stage 2

- Convert the idea into measurable functional and non-functional requirements
- Split the system into modules and define deliverables
- Prepare a day-by-day development plan up to the 5 October deadline
