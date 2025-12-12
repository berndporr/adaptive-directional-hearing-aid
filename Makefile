# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS = -lasound

# Source files
SRCS = play_plain_audio.cpp ALSADevices.cpp Fir1.cpp
OBJS = $(SRCS:.cpp=.o)

# Output executable
TARGET = out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
