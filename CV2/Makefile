# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -fPIC -g

# Program executables
GENERATOR = gen
SUM_CHECKER = sum

# Library names
STATIC_LIB = libstatic.a
DYNAMIC_LIB = libdynamic.so

# Source files
GEN_SRC = gen.cpp
SUM_SRC = sum.cpp
STATIC_LIB_SRC = libSta.cpp
DYNAMIC_LIB_SRC = libDyn.cpp

# Object files
GEN_OBJ = gen.o
SUM_OBJ = sum.o
STATIC_LIB_OBJ = libSta.o
DYNAMIC_LIB_OBJ = libDyn.o

# Default target
all: $(GENERATOR) $(SUM_CHECKER) $(STATIC_LIB) $(DYNAMIC_LIB)

# Compile the generator program
$(GENERATOR): $(GEN_OBJ) $(STATIC_LIB)
	$(CXX) $(GEN_OBJ) -L. -lstatic -o $@ 

# Compile the sum checker program (with dynamic library loading)
$(SUM_CHECKER): $(SUM_OBJ)
	$(CXX) $(SUM_OBJ) -ldl -o $@

# Build the static library
$(STATIC_LIB): $(STATIC_LIB_OBJ)
	ar rcs $(STATIC_LIB) $(STATIC_LIB_OBJ)

# Build the dynamic library
$(DYNAMIC_LIB): $(DYNAMIC_LIB_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $(DYNAMIC_LIB) $(DYNAMIC_LIB_OBJ)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(GENERATOR) $(SUM_CHECKER) *.o $(STATIC_LIB) $(DYNAMIC_LIB)
