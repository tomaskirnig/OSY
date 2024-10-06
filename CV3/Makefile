CXX = g++

CXXFLAGS = -Wall -g

# Define targets
TARGETS = myls mygen monitor

# Source files for each target
SRCS_myls = myls.cpp
SRCS_mygen = mygen.cpp
SRCS_monitor = monitor.cpp

# Object files for each target
OBJS_myls = $(SRCS_myls:.cpp=.o)
OBJS_mygen = $(SRCS_mygen:.cpp=.o)
OBJS_monitor = $(SRCS_monitor:.cpp=.o)

all: $(TARGETS)

# Rule to link the object files to create the myls executable
myls: $(OBJS_myls)
	$(CXX) $(CXXFLAGS) -o myls $(OBJS_myls)

# Rule to link the object files to create the mygen executable
mygen: $(OBJS_mygen)
	$(CXX) $(CXXFLAGS) -o mygen $(OBJS_mygen)

monitor: $(OBJS_monitor)
	$(CXX) $(CXXFLAGS) -o monitor $(OBJS_monitor)

# Rule to compile source files into object files for any .cpp file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f *.o $(TARGETS)
