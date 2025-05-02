
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
TARGET = main

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -g

clean:
	rm -f $(TARGET)