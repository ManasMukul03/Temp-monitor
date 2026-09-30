# Virtual Temperature Monitoring System

A Linux character device driver that simulates a temperature sensor (`/dev/tempsensor`),
and a multithreaded C++ application that monitors it in real time.

**Author:** Manas Mukul — Wipro Training, Batch 3

## Topics covered
Linux · Device Drivers · System Programming · C++ · Computer Architecture · Hardware & Software

## Repository structure
```
driver/   Linux kernel module (C)
app/      C++ monitoring application
tests/    Test programs
docs/     Stage documents, UML diagrams
PROGRESS.md  Daily progress log
```

## Status
- [x] Stage 1 – Project Introduction
- [x] Stage 2 – Requirements & Development Plan
- [x] Stage 3 – System Design & Architecture
- [ ] Stage 4 – Initial Implementation & Prototype
- [ ] Stage 5 – Testing, Integration & Improvement
- [ ] Stage 6 – Final Implementation & Presentation

## Build & run the driver
```bash
cd driver
make                      # builds tempsensor.ko
sudo insmod tempsensor.ko # load  (creates /dev/tempsensor)
cat /dev/tempsensor       # one reading in milli-degC
cat /proc/tempsensor      # register dump
sudo dmesg | tail         # driver messages
sudo rmmod tempsensor     # unload
```

## Driver test
```bash
cd tests
g++ -std=c++17 -Wall -I../driver driver_test.cpp -o driver_test
./driver_test
```
