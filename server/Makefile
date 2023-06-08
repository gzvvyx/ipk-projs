CXX = g++
CXXFLAGS = -pedantic -std=c++20

SRCS = ipkpd.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = ipkpd

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

runtcp: all
	./$(TARGET) -h 127.0.0.1 -p 2020 -m tcp

runudp: all
	./$(TARGET) -h 127.0.0.1 -p 3030 -m udp

clean:
	$(RM) $(OBJS) $(TARGET)
