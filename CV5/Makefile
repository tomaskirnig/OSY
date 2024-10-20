CXX = g++
CXXFLAGS = -Wall -g

TARGET = sort

TARGET_FILE = $(TARGET).cpp

all: $(TARGET)

$(TARGET): $(TARGET_FILE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TARGET_FILE)

clean:
	rm -f $(TARGET)