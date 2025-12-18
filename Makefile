# Cross-compilation configuration
CROSS_COMPILE ?= aarch64-linux-gnu-
SYSROOT ?= /home/lese/ti-processor-sdk-linux-am62lxx-evm-11.01.16.13/linux-devkit/sysroots/aarch64-oe-linux

CXX = $(CROSS_COMPILE)g++
CXXFLAGS = -Wall -O2 -std=c++11 --sysroot=$(SYSROOT)
LDFLAGS = --sysroot=$(SYSROOT)

TARGET = epaper_test
SRCS = main.cpp EinkDisplay.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
