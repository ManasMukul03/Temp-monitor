# Virtual Temperature Monitor - C++ application
#   make        -> builds ./tempmon
#   make test   -> builds and runs the unit tests (no driver needed)
#   make clean
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -I../driver
SOURCES  = Types.cpp SensorDevice.cpp AlertManager.cpp CoolingController.cpp \
           TemperatureHistory.cpp LoggerProcess.cpp MonitorEngine.cpp \
           SystemInfo.cpp ReportExporter.cpp
HEADERS  = $(wildcard *.h) ../driver/tempsensor_ioctl.h

all: tempmon

tempmon: main.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp $(SOURCES) -o tempmon

test: ../tests/app_test.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -I. ../tests/app_test.cpp $(SOURCES) -o ../tests/app_test
	../tests/app_test

clean:
	rm -f tempmon ../tests/app_test *.log *.csv

.PHONY: all test clean
