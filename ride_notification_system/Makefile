CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS  = -pthread
TARGET   = ride_notification_system

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET)

.PHONY: clean
